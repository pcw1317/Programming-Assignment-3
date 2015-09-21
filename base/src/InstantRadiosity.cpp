#include "InstantRadiosity.h"
#include <glm/gtx/rotate_vector.hpp>

struct Vertex { float x, y, z, r; };
struct Triangle { int v0, v1, v2; };

void InstantRadiosityEmbree::addMesh(Mesh mesh)
{
	unsigned int geomID = rtcNewTriangleMesh(scene, RTC_GEOMETRY_STATIC, 
		mesh.indices.size() / 3, mesh.vertices.size());

	Vertex *vertices = (Vertex *) rtcMapBuffer(scene, geomID, RTC_VERTEX_BUFFER);
	unsigned int vertexIdx = 0;
	for (auto vertex : mesh.vertices)
	{
		vertices[vertexIdx].x = vertex.x; vertices[vertexIdx].y = vertex.y; vertices[vertexIdx].z = vertex.z;
		vertexIdx++;
	}
	rtcUnmapBuffer(scene, geomID, RTC_VERTEX_BUFFER);

	Triangle *triangles = (Triangle *) rtcMapBuffer(scene, geomID, RTC_INDEX_BUFFER);
	for (int triIdx = 0; triIdx < mesh.indices.size() / 3; ++triIdx)
	{
		triangles[triIdx].v0 = mesh.indices[triIdx * 3];
		triangles[triIdx].v1 = mesh.indices[triIdx * 3 + 2];
		triangles[triIdx].v2 = mesh.indices[triIdx * 3 + 1];
	}
	rtcUnmapBuffer(scene, geomID, RTC_INDEX_BUFFER);

	geomIDToMesh[geomID] = mesh;
}

std::vector<LightData> InstantRadiosityEmbree::getVPLpos(LightData light, unsigned int rayCount, unsigned int recursionDepth)
{
	std::vector<LightData> res;

	res.push_back(light);	// add itself also as the VPL

	for (int rayIter = 0; rayIter < rayCount; ++rayIter)
	{
		// should be a proper stratified quasirandom sampling direction
		glm::vec3 randomDir = stratifiedSampling(light.direction, rayIter, rayCount);

		// Ray init.
		RTCRay ray;
		ray.org[0] = light.position.x; ray.org[1] = light.position.y; ray.org[2] = light.position.z;
		ray.dir[0] = randomDir.x; ray.dir[1] = randomDir.y; ray.dir[2] = randomDir.z;
		ray.tnear = 0.f;
		ray.tfar = INFINITY;
		ray.geomID = RTC_INVALID_GEOMETRY_ID;
		ray.primID = RTC_INVALID_GEOMETRY_ID;
		ray.instID = RTC_INVALID_GEOMETRY_ID;
		ray.mask = 0xFFFFFFFF;
		ray.time = 0.f;

		// Intersect!
		rtcIntersect(scene, ray);

		glm::vec3 rayOrg = glm::vec3(ray.org[0], ray.org[1], ray.org[2]);
		glm::vec3 rayDir = glm::vec3(ray.dir[0], ray.dir[1], ray.dir[2]);
		glm::vec3 rayNg = glm::normalize(glm::vec3(ray.Ng[0], ray.Ng[1], ray.Ng[2]));

		// Made an intersection
		if (ray.geomID != RTC_INVALID_GEOMETRY_ID)
		{
			LightData VPL;
			VPL.position = rayOrg + rayDir * ray.tfar;
			VPL.intensity = light.intensity *	// light color
				geomIDToMesh[ray.geomID].color *	// diffuse only
				glm::abs(glm::dot(rayNg, rayDir)) *	// Lambertian cosine term
				/*glm::pow((ray.tfar - ray.tnear), -2.f)*/1.f;	// d^-2
			VPL.direction = rayNg;
			// recurse to make a global illumination
			if (recursionDepth > 0)
			{
				std::vector<LightData> vpls = getVPLpos(VPL, rayCount, recursionDepth - 1);
				res.insert(res.end(), vpls.begin(), vpls.end());
			}
			else
			{
				res.push_back(VPL);
			}
		}
	}

	return res;
}

std::vector<LightData> InstantRadiosityEmbree::getVPLpos(AreaLightData light, unsigned int sampleCount, unsigned int rayCount, unsigned int recursionDepth)
{
	std::vector<LightData> res;

	for (int sampleIter = 0; sampleIter < sampleCount; ++sampleIter)
	{
		glm::vec3 lightPos = stratifiedSampling(light.lightMin, light.lightMax, sampleIter, sampleCount);
		LightData lightSample;
		lightSample.position = lightPos;
		lightSample.direction = light.direction;
		lightSample.intensity = light.intensity;
		std::vector<LightData> vpls = getVPLpos(lightSample, rayCount, recursionDepth);
		res.insert(res.end(), vpls.begin(), vpls.end());
	}

	return res;
}

const float PI = 3.14159265358979;

glm::vec3 InstantRadiosityEmbree::stratifiedSampling(glm::vec3 normalVec, unsigned int current, unsigned int total)
{
	glm::vec2 hammersley = hammersley2d(current, total);
	float phi = hammersley.y * 2.0 * PI;
	float cosTheta = 1.0 - hammersley.x;
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// point over hemisphere with normal vector (0, 0, 1)
	glm::vec3 point = glm::vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

	glm::vec3 perpVec1 = glm::normalize(glm::cross(glm::vec3(0, -normalVec.z, normalVec.y), normalVec));
	glm::vec3 perpVec2 = glm::cross(perpVec1, normalVec);
	
	return point.x * perpVec1 + point.y * perpVec2 + point.z * normalVec;
}

glm::vec3 InstantRadiosityEmbree::stratifiedSampling(glm::vec3 bbMin, glm::vec3 bbMax, unsigned int current, unsigned int total)
{
	// to be implemented
	return (bbMin + bbMax) / 2.0f;
}

float InstantRadiosityEmbree::radicalInverse_VdC(unsigned int bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

glm::vec2 InstantRadiosityEmbree::hammersley2d(unsigned int i, unsigned int N)
{
	return glm::vec2(float(i) / float(N), radicalInverse_VdC(i));
}

// Gives a random direction over the hemisphere above the surface with the normal "normal".
// v1 and v2 are to be in the interval [0,1].
// If normal is of length 0, then this gives a random direction in a sphere centered around the point.
glm::vec3 randDirHemisphere (glm::vec3 normal, float v1, float v2) 
{    
    float cosPhi = sqrt (v1);		
    float sinPhi = sqrt (1.0 - v1);	
    float theta = v2 * 2.0 * 3.141592;

	if (glm::dot (normal, normal) < 0.0001f)
		return glm::vec3 (sinPhi*cos(theta), cosPhi, sinPhi*sin(theta));
        
	glm::vec3 someDirNotNormal;
    if ((normal.x < normal.y) && (normal.x < normal.z)) 
      someDirNotNormal = glm::vec3 (1.0, 0.0, 0.0);
    else if (normal.y < normal.z)
      someDirNotNormal = glm::vec3 (0.0, 1.0, 0.0);
    else
      someDirNotNormal = glm::vec3 (0.0, 0.0, 1.0);
    
    glm::vec3 basis1 = glm::normalize (glm::cross (normal, someDirNotNormal));
    glm::vec3 basis2 = glm::normalize (glm::cross (normal, basis1));
    
    return (cosPhi * normal) + (sinPhi*cos (theta) * basis1) + (sinPhi*sin (theta) * basis2);    
}