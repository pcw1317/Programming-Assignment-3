#pragma once

#include <vector>
#include "tiny_obj_loader.h"
#include "Camera.h"

class DeviceMesh;

class SystemContext
{
public:
	SystemContext()
	{
		pCam = new Camera(glm::vec3(2.5, 5, 2),
			glm::normalize(glm::vec3(0, -1, 0)),
			glm::normalize(glm::vec3(0, 0, 1)));
		viewport.x = 1280;
		viewport.y = 720;
	}
	~SystemContext()
	{
		delete pCam;
	}

	std::string LoadObj(char *path);
	void initMesh();
	std::vector<tinyobj::shape_t>::iterator shapesBeginIter() { return shapes.begin(); }
	std::vector<tinyobj::shape_t>::iterator shapesEndIter() { return shapes.end(); }

protected:
	std::vector<tinyobj::shape_t> shapes;

public:
	std::vector<DeviceMesh> drawMeshes;
	Camera *pCam;
	glm::ivec2 viewport;
};