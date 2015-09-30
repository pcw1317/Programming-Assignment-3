#include "raytracer.h"
#include <memory>
#include <glm/gtc/type_ptr.hpp>
#include "random.h"

namespace {
	std::unique_ptr<bool> raytracer_singleton;
	void error_handler(const RTCError code, const char *str)
	{
		printf("Embree error: ");
		switch (code)
		{
		case RTC_UNKNOWN_ERROR:
			printf("RTC_UNKNOWN_ERROR");
			break;
		case RTC_INVALID_ARGUMENT:
			printf("RTC_INVALID_ARGUMENT");
			break;
		case RTC_INVALID_OPERATION:
			printf("RTC_INVALID_OPERATION");
			break;
		case RTC_OUT_OF_MEMORY:
			printf("RTC_OUT_OF_MEMORY");
			break;
		case RTC_UNSUPPORTED_CPU:
			printf("RTC_UNSUPPORTED_CPU");
			break;
		case RTC_CANCELLED:
			printf("RTC_CANCELLED");
			break;
		default:
			printf("invalid error code");
			break;
		}
		if (str)
		{
			printf(" (");
			while (*str) putchar(*str++);
			printf(")\n");
		}
		exit(1);
	}
}

constexpr float PI = 3.14159265358979f;
constexpr float kJitterEpsilon = 0.2f;
constexpr float kRayTraceEpsilon = 0.01f;

struct Vertex
{
    float x, y, z, r;
};
struct Triangle
{
    int v0, v1, v2;
};

raytracer::raytracer(): uniform_real_distribution_jitter_(-kJitterEpsilon, kJitterEpsilon), uniform_real_distribution_01_(0.f, 1.f) {
	assert(raytracer_singleton.get() == nullptr); // assure only one instance of raytracer can be there

	rtcInit(NULL);
#ifdef _DEBUG
	rtcSetErrorFunction(error_handler);
#endif
	scene_ = rtcNewScene(RTC_SCENE_STATIC, RTC_INTERSECT1);
}

raytracer::~raytracer() {
	rtcDeleteScene(scene_);
	rtcExit();
}

void raytracer::commit_scene() { rtcCommit(scene_); }

void raytracer::add_mesh( host_mesh_t mesh )
{
    unsigned int geomID = rtcNewTriangleMesh( scene_, RTC_GEOMETRY_STATIC,
                          mesh.indices.size() / 3, mesh.vertices.size() );

    Vertex *vertices = ( Vertex * ) rtcMapBuffer( scene_, geomID, RTC_VERTEX_BUFFER );
    unsigned int vertexIdx = 0;
    for( auto vertex : mesh.vertices )
    {
        vertices[vertexIdx].x = vertex.x;
        vertices[vertexIdx].y = vertex.y;
        vertices[vertexIdx].z = vertex.z;
        vertexIdx++;
    }
    rtcUnmapBuffer( scene_, geomID, RTC_VERTEX_BUFFER );

    Triangle *triangles = ( Triangle * ) rtcMapBuffer( scene_, geomID, RTC_INDEX_BUFFER );
    for( int triIdx = 0; triIdx < mesh.indices.size() / 3; ++triIdx )
    {
        triangles[triIdx].v0 = mesh.indices[triIdx * 3];
        triangles[triIdx].v1 = mesh.indices[triIdx * 3 + 2];
        triangles[triIdx].v2 = mesh.indices[triIdx * 3 + 1];
    }
    rtcUnmapBuffer( scene_, geomID, RTC_INDEX_BUFFER );

    geomIDToMesh[geomID] = mesh;
}

std::vector<point_light_t> raytracer::compute_vpl( point_light_t light, unsigned int recursion_depth_left) {
	
    std::vector<point_light_t> res;
    res.push_back( light );	// add itself as the VPL first

	if (recursion_depth_left <= 0) //preventing stack overflow
		return res;

    // should be a proper stratified quasirandom sampling direction
    glm::vec3 randomDir = random::stratified_sampling( light.direction, [&]() {return uniform_real_distribution_01_(random_engine_); });

    // Ray init.
    RTCRay ray;
    std::memcpy(ray.org, glm::value_ptr(light.position), sizeof(ray.org));
	std::memcpy(ray.dir, glm::value_ptr(randomDir), sizeof(ray.dir));
    ray.tnear = kRayTraceEpsilon; ray.tfar = INFINITY;
    ray.geomID = RTC_INVALID_GEOMETRY_ID;
    ray.primID = RTC_INVALID_GEOMETRY_ID;
    ray.instID = RTC_INVALID_GEOMETRY_ID;
    ray.mask = 0xFFFFFFFF;
    ray.time = 0.f;

    // Intersect!
    rtcIntersect( scene_, ray );

    // skip if missed
    if( ray.geomID == RTC_INVALID_GEOMETRY_ID )
		return res;
	// skip if russian roulette failed
	float rr_probability = (geomIDToMesh[ray.geomID].diffuse_color.x + geomIDToMesh[ray.geomID].diffuse_color.y + geomIDToMesh[ray.geomID].diffuse_color.z) / 3;
	//float rr_probability = glm::length(light.intensity) / 15.f;
	if (rr_probability <= uniform_real_distribution_01_(random_engine_))
		return res;
	
	//trace new one
	glm::vec3 rayOrg = glm::vec3(ray.org[0], ray.org[1], ray.org[2]);
	glm::vec3 rayDir = glm::vec3(ray.dir[0], ray.dir[1], ray.dir[2]);
	glm::vec3 rayNg = glm::normalize(glm::vec3(ray.Ng[0], ray.Ng[1], ray.Ng[2]));

	point_light_t VPL;
	VPL.position = rayOrg + rayDir * ray.tfar;
	VPL.intensity = light.intensity	// light color
		* geomIDToMesh[ray.geomID].diffuse_color	// diffuse only
		* glm::abs(glm::dot(rayNg, rayDir))
		/ PI / rr_probability;	// russian roulette weight
	VPL.direction = rayNg;

	// recurse to make a global illumination
	std::vector<point_light_t> vpls = compute_vpl(VPL, recursion_depth_left - 1);
	res.insert(res.end(), vpls.begin(), vpls.end());

    return res;
}

std::vector<point_light_t> raytracer::compute_vpl( area_light_t light, unsigned int light_sample_count)
{
    std::vector<point_light_t> res;

    for( unsigned int light_sample_idx = 0; light_sample_idx < light_sample_count; ++light_sample_idx)
    {
        glm::vec3 lightPos = random::stratified_sampling( light.aabb_min, light.aabb_max, light_sample_idx, light_sample_count, [&]() {return uniform_real_distribution_jitter_(random_engine_); });
        point_light_t lightSample;
        lightSample.position = lightPos;
        lightSample.direction = light.direction;
		
		glm::vec2 area = (light.aabb_max - light.aabb_min).xz();
		lightSample.intensity =
			light.intensity					// L(x)
			* (area.x * area.y)				// 1/pdf of light sampling
			/ float(light_sample_count);	// Monte Carlo integration divisor
        
		std::vector<point_light_t> vpls = compute_vpl( lightSample );
        res.insert( res.end(), vpls.begin(), vpls.end() );
    }

    return res;
}

// Gives a random direction over the hemisphere above the surface with the normal "normal".
// v1 and v2 are to be in the interval [0,1].
// If normal is of length 0, then this gives a random direction in a sphere centered around the point.
glm::vec3 randDirHemisphere( glm::vec3 normal, float v1, float v2 )
{
    float cosPhi = sqrt( v1 );
    float sinPhi = sqrt( 1.0f - v1 );
    float theta = v2 * 2.0f * PI;

    if( glm::dot( normal, normal ) < 0.0001f )
        return glm::vec3( sinPhi * cos( theta ), cosPhi, sinPhi * sin( theta ) );

    glm::vec3 someDirNotNormal;
    if( ( normal.x < normal.y ) && ( normal.x < normal.z ) )
        someDirNotNormal = glm::vec3( 1.0, 0.0, 0.0 );
    else if( normal.y < normal.z )
        someDirNotNormal = glm::vec3( 0.0, 1.0, 0.0 );
    else
        someDirNotNormal = glm::vec3( 0.0, 0.0, 1.0 );

    glm::vec3 basis1 = glm::normalize( glm::cross( normal, someDirNotNormal ) );
    glm::vec3 basis2 = glm::normalize( glm::cross( normal, basis1 ) );

    return ( cosPhi * normal ) + ( sinPhi * cos( theta ) * basis1 ) + ( sinPhi * sin( theta ) * basis2 );
}