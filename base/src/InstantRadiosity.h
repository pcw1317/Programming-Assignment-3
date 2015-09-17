#pragma once

#include <glm/glm.hpp>
#include <rtcore.h>
#include <rtcore_ray.h>
#include "Mesh.h"
#include <vector>

struct LightData
{
	glm::vec3 position;
	glm::vec3 intensity;
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
	std::vector<LightData> getVPLposPointLight(glm::vec3 pointLightPos, glm::vec3 lightNormalVec, unsigned int count);
	std::vector<LightData> getVPLposAreaLight(glm::vec3 areaLightMin, glm::vec3 areaLightMax, unsigned int count);

protected:
	std::vector<unsigned int> geomIDs;
	std::map<unsigned int, Mesh *> geomIDToMesh;

	glm::vec3 stratifiedSampling(glm::vec3 normalVec);
};

struct Ray
{
	glm::vec4 origin;
	glm::vec4 direction;
	glm::vec4 intensity;
};

glm::vec3 randDirHemisphere (glm::vec3 normal, float v1, float v2);
