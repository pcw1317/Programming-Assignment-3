#include "Camera.h"

#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtx/euler_angles.hpp>

// yaw-pitch-roll
void Camera::rotate(glm::vec3 eulerAngleDelta)
{
	eulerAngle += eulerAngleDelta;
}

void Camera::translate(glm::vec3 distance)
{
	pos += distance;
}

glm::mat4x4 Camera::get_view()
{
	glm::mat4 rotMat = glm::gtx::euler_angles::orientate4(eulerAngle);
	glm::vec3 lookingAtDir = glm::vec3(glm::vec4(initLookingAtDir, 0) * rotMat);
	glm::vec3 up = glm::vec3(glm::vec4(initUp, 0) * rotMat);
	return glm::gtx::transform2::lookAt(pos, pos + lookingAtDir, up);
}