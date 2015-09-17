#include "InstantRadiosity.h"

void InstantRadiosityEmbree::addMesh(const Mesh & mesh)
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