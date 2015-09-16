#pragma once

#include <glm/glm.hpp>
#include <rtcore.h>
#include <rtcore_ray.h>
#include "Mesh.h"

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

	void addMesh(const Mesh &mesh);
	std::vector<glm::vec3> getVPLpos(glm::vec3 pointLightPos, unsigned int count);
	std::vector<glm::vec3> getVPLpos(glm::vec3 areaLightMin, glm::vec3 areaLightMax, unsigned int count);

protected:
	std::vector<unsigned int> geomIDs;
	std::map<unsigned int, Mesh &> geomIDToMesh;
};

struct LightData
{
	glm::vec4 position;
	glm::vec4 intensity;
};

struct Ray
{
	glm::vec4 origin;
	glm::vec4 direction;
	glm::vec4 intensity;
};

glm::vec3 randDirHemisphere (glm::vec3 normal, float v1, float v2);
