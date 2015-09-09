#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "tiny_obj_loader/tiny_obj_loader.h"

class Mesh
{
public:
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texcoords;
	std::vector<unsigned short> indices;
	std::string texname;
	glm::vec3 color;

public:
	Mesh(std::vector<glm::vec3> &vertices,
		std::vector<glm::vec3> &normals,
		std::vector<glm::vec2> &texcoords,
		std::vector<unsigned short> &indices,
		std::string &texname,
		glm::vec3 &color)
	{
		this->vertices = std::vector<glm::vec3>(vertices);
		this->normals = std::vector<glm::vec3>(normals);
		this->texcoords = std::vector<glm::vec2>(texcoords);
		this->indices = std::vector<unsigned short>(indices);
		this->texname = std::string(texname);
		this->color = glm::vec3(color);
	}

	static Mesh meshFromShape(const tinyobj::shape_t &shape);
};