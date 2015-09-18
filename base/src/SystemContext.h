#pragma once

#include <vector>
#include <cassert>
#include <memory>
#include "tiny_obj_loader.h"
#include "Camera.h"
#include <exception>
#include "InstantRadiosity.h"

class DeviceMesh;

class SystemContext
{
public:
	~SystemContext()
	{
		delete irKernel;
	}

	void loadObj(char *path);
	void initMesh();
	std::vector<tinyobj::shape_t>::iterator shapesBeginIter() { return shapes.begin(); }
	std::vector<tinyobj::shape_t>::iterator shapesEndIter() { return shapes.end(); }

protected:
	SystemContext(const Camera &pCam, const glm::uvec2 &viewport) :
		pCam(pCam), viewport(viewport)
	{
		irKernel = new InstantRadiosityEmbree();
	}
	std::vector<tinyobj::shape_t> shapes;
	InstantRadiosityEmbree *irKernel;
	AreaLightData light;
	
public:
	std::vector<DeviceMesh> drawMeshes;
	Camera pCam;
	glm::uvec2 viewport;

public:
	static std::unique_ptr<SystemContext> gContext;

	template<typename... Args>
	static SystemContext* initialize(Args&&... args) {
		assert(gContext.get() == nullptr);
		gContext.reset(new SystemContext(std::forward<Args>(args)...));
		return gContext.get();
	}
};
