#pragma once

#include <glm/glm.hpp>

class Light
{
public:
	Light(glm::vec3 start_pos, glm::vec3 start_dir, glm::vec3 up) :
		pos(start_pos.x, start_pos.y), z(start_pos.z), up(up),
		start_dir(start_dir), start_left(glm::cross(start_dir, up)), rx(0), ry(0)
	{ }

	glm::mat4x4 get_light_view();

	float rx;
	float ry;
	float z;
	glm::vec2 pos;
	glm::vec3 up;
	glm::vec3 start_left;
	glm::vec3 start_dir;
};