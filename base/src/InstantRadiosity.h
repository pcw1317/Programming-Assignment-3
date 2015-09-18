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
		rtcSetErrorFunction(InstantRadiosityEmbree::error_handler);
		scene = rtcNewScene(RTC_SCENE_STATIC, RTC_INTERSECT1);
	}

	~InstantRadiosityEmbree()
	{
		rtcDeleteScene(scene);
		rtcExit();
	}

	void addMesh(Mesh mesh);
	std::vector<LightData> getVPLpos(LightData light, unsigned int count);
	std::vector<LightData> getVPLpos(AreaLightData light, unsigned int count);
	
protected:
	std::vector<unsigned int> geomIDs;
	std::map<unsigned int, Mesh> geomIDToMesh;

	glm::vec3 stratifiedSampling(glm::vec3 normalVec);
	glm::vec3 stratifiedSampling(glm::vec3 bbMin, glm::vec3 bbMax);

	static void error_handler(const RTCError code, const char* str)
	{
		printf("Embree: ");
		switch (code) {
		case RTC_UNKNOWN_ERROR: printf("RTC_UNKNOWN_ERROR"); break;
		case RTC_INVALID_ARGUMENT: printf("RTC_INVALID_ARGUMENT"); break;
		case RTC_INVALID_OPERATION: printf("RTC_INVALID_OPERATION"); break;
		case RTC_OUT_OF_MEMORY: printf("RTC_OUT_OF_MEMORY"); break;
		case RTC_UNSUPPORTED_CPU: printf("RTC_UNSUPPORTED_CPU"); break;
		case RTC_CANCELLED: printf("RTC_CANCELLED"); break;
		default: printf("invalid error code"); break;
		}
		if (str) {
			printf(" (");
			while (*str) putchar(*str++);
			printf(")\n");
		}
		exit(1);
	}
};

struct Ray
{
	glm::vec4 origin;
	glm::vec4 direction;
	glm::vec4 intensity;
};

glm::vec3 randDirHemisphere (glm::vec3 normal, float v1, float v2);
