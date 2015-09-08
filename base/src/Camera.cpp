#include "Camera.h"

#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform2.hpp>

void Camera::adjust(float dx, // look left right
	float dy, //look up down
	float dz,
	float tx, //strafe left right
	float ty,
	float tz)//go forward) //strafe up down
{
	if (abs(dx) > 0)
	{
		rx += dx;
		rx = fmod(rx, 360.0f);
	}

	if (abs(dy) > 0)
	{
		ry += dy;
		ry = glm::clamp(ry, -70.0f, 70.0f);
	}

	if (abs(tx) > 0)
	{
		glm::vec3 dir = glm::rotate(start_dir, rx + 90, up);
		glm::vec2 dir2(dir.x, dir.y);
		glm::vec2 mag = dir2 * tx;
		pos += mag;
	}

	if (abs(ty) > 0)
	{
		z += ty;
	}

	if (abs(tz) > 0)
	{
		glm::vec3 dir = glm::rotate(start_dir, rx, up);
		glm::vec2 dir2(dir.x, dir.y);
		glm::vec2 mag = dir2 * tz;
		pos += mag;
	}
}

glm::mat4x4 Camera::get_view()
{
	glm::vec3 inclin = glm::rotate(start_dir, ry, start_left);
	glm::vec3 spun = glm::rotate(inclin, rx, up);
	glm::vec3 cent(pos, z);
	return glm::gtx::transform2::lookAt(cent, cent + spun, up);
}