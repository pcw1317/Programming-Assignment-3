#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
	Camera(glm::vec3 start_pos, glm::vec3 start_dir, glm::vec3 up) :
		pos(start_pos), initUp(up), initLookingAtDir(glm::normalize(start_dir))
	{ }

	void rotate(glm::vec3 eulerAngleDelta);
	void translate(glm::vec3 distance);
	glm::mat4x4 get_view();
	glm::mat4 get_perspective() { return perspectiveMat; }
	void set_perspective(const glm::mat4 &persp) { perspectiveMat = persp; }

	glm::vec3 eulerAngle;
	glm::vec3 pos;
	glm::vec3 initUp;
	glm::vec3 initLookingAtDir;

protected:
	glm::mat4 perspectiveMat;
};