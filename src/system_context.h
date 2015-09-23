#pragma once

#include <vector>
#include <cassert>
#include <memory>
#include <tiny_obj_loader/tiny_obj_loader.h>
#include "Camera.h"
#include <exception>
#include "InstantRadiosity.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "gl_snippets.h"

class DeviceMesh;

class system_context
{
public:
	~system_context()
	{
		delete irKernel;
	}

	void loadObj(char *path);
	void initMesh();
	std::vector<tinyobj::shape_t>::iterator shapesBeginIter() { return shapes.begin(); }
	std::vector<tinyobj::shape_t>::iterator shapesEndIter() { return shapes.end(); }

protected:
	system_context(const Camera &pCam, const glm::uvec2 &viewport) :
		pCam(pCam), viewport(viewport)
	{
		irKernel = new InstantRadiosityEmbree();
	}
	std::vector<tinyobj::shape_t> shapes;
	InstantRadiosityEmbree *irKernel;
	AreaLightData light;
	
public:
	std::vector<DeviceMesh> drawMeshes;
	GLFWwindow *window;
	Camera pCam;
	glm::uvec2 viewport;
	std::vector<LightData> VPLs;

	std::vector<gls::program> gls_programs;
	std::vector<gls::buffer> gls_buffers;
	std::vector<gls::vertex_array> gls_vertex_arrays;
	std::vector<gls::framebuffer<gls::texture, gls::texture> > gls_framebuffers;

protected:
	static std::unique_ptr<system_context> global_context_;
public:
	template<typename... Args>
	static system_context* initialize(Args&&... args) {
		assert(global_context_.get() == nullptr);
		global_context_.reset(new system_context(std::forward<Args>(args)...));
		return global_context_.get();
	}
	static system_context* get() {
		assert(global_context_.get() != nullptr);
		return global_context_.get();
	}
};
