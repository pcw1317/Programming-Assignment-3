#include "Light.h"

#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform2.hpp>

glm::mat4x4 Light::get_light_view()
{
	glm::vec3 inclin = glm::rotate(start_dir, ry, start_left);
	glm::vec3 spun = glm::rotate(inclin, rx, up);
	glm::vec3 cent(pos, z);
	return glm::lookAt(cent, cent + spun, up);
}