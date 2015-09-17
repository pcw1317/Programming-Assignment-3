#include "InstantRadiosity.h"

void InstantRadiosityEmbree::addMesh(Mesh &mesh)
{
	unsigned int geomID = rtcNewTriangleMesh(scene, RTC_GEOMETRY_DYNAMIC, 
		mesh.indices.size() / 3, mesh.vertices.size());

	glm::vec3 *vertices = (glm::vec3 *) rtcMapBuffer(scene, geomID, RTC_VERTEX_BUFFER);
	unsigned int vertexIdx = 0;
	for (auto vertex : mesh.vertices)
	{
		vertices[vertexIdx++] = glm::vec3(vertex);
	}
	rtcUnmapBuffer(scene, geomID, RTC_VERTEX_BUFFER);

	glm::ivec3 *triangles = (glm::ivec3 *) rtcMapBuffer(scene, geomID, RTC_INDEX_BUFFER);
	for (int triIdx = 0; triIdx < mesh.indices.size() / 3; ++triIdx)
	{
		triangles[triIdx].x = mesh.indices[triIdx * 3];
		triangles[triIdx].y = mesh.indices[triIdx * 3 + 1];
		triangles[triIdx].z = mesh.indices[triIdx * 3 + 2];
	}
	rtcUnmapBuffer(scene, geomID, RTC_INDEX_BUFFER);

	geomIDToMesh[geomID] = &mesh;
}

std::vector<LightData> InstantRadiosityEmbree::getVPLposPointLight(glm::vec3 pointLightPos, glm::vec3 lightNormalVec, unsigned int count)
{
	std::vector<LightData> res;

	for (int rayCount = 0; rayCount < count; ++rayCount)
	{
		// should be a proper stratified quasirandom sampling direction
		glm::vec3 randomDir = stratifiedSampling(lightNormalVec);

		// Ray init.
		RTCRay ray;
		ray.org[0] = pointLightPos.x; ray.org[1] = pointLightPos.y; ray.org[2] = pointLightPos.z;
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
		glm::vec3 rayNg = glm::vec3(ray.Ng[0], ray.Ng[1], ray.Ng[2]);

		// Made an intersection
		if (ray.geomID != RTC_INVALID_GEOMETRY_ID)
		{
			LightData VPL;
			VPL.position = rayOrg + rayDir * ray.tfar;
			VPL.intensity = geomIDToMesh[ray.geomID]->color *	// diffuse only
				glm::dot(rayNg, rayDir) *	// Lambertian cosine term
				glm::pow((ray.tfar - ray.tnear), -2.f);	// d^-2
			res.push_back(VPL);
		}
	}

	return res;
}

glm::vec3 InstantRadiosityEmbree::stratifiedSampling(glm::vec3 normalVec)
{
	// to be implemented
	return glm::vec3();
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