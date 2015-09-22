#pragma once

#include <vector>
#include <cassert>
#include <memory>
#include <tiny_obj_loader/tiny_obj_loader.h>
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
	std::vector<LightData> VPLs;

protected:
	static std::unique_ptr<SystemContext> global_context_;
public:
	template<typename... Args>
	static SystemContext* initialize(Args&&... args) {
		assert(global_context_.get() == nullptr);
		global_context_.reset(new SystemContext(std::forward<Args>(args)...));
		return global_context_.get();
	}
	static SystemContext* get() {
		assert(global_context_.get() != nullptr);
		return global_context_.get();
	}
};
