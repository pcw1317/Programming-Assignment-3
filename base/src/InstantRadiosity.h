#pragma once

#include <glm/glm.hpp>
#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>
#include "Mesh.h"
#include <vector>

struct LightData
{
	glm::vec3 position;
	glm::vec3 intensity;
	glm::vec3 direction;
};

struct AreaLightData
{
	glm::vec3 lightMin;
	glm::vec3 lightMax;
	glm::vec3 intensity;
	glm::vec3 direction;
};

class InstantRadiosityEmbree
{
	RTCScene scene;

public:
	InstantRadiosityEmbree()
	{
		rtcInit(NULL);
		scene = rtcNewScene(RTC_SCENE_STATIC, RTC_INTERSECT1);
	}

	~InstantRadiosityEmbree()
	{
		rtcDeleteScene(scene);
		rtcExit();
	}

	void addMesh(Mesh &mesh);
	std::vector<LightData> getVPLpos(LightData light, unsigned int count);
	std::vector<LightData> getVPLpos(AreaLightData light, unsigned int count);
	
protected:
	std::vector<unsigned int> geomIDs;
	std::map<unsigned int, Mesh *> geomIDToMesh;

	glm::vec3 stratifiedSampling(glm::vec3 normalVec);
	glm::vec3 stratifiedSampling(glm::vec3 bbMin, glm::vec3 bbMax);
};

struct Ray
{
	glm::vec4 origin;
	glm::vec4 direction;
	glm::vec4 intensity;
};

glm::vec3 randDirHemisphere (glm::vec3 normal, float v1, float v2);
