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
	Mesh()
	{
		AABBcalculated = false;
		AABBmin = glm::vec3(1e300);
		AABBmax = glm::vec3(-1e300);
	}

	Mesh(Mesh &mesh)
	{
		this->vertices = std::vector<glm::vec3>(mesh.vertices);
		this->normals = std::vector<glm::vec3>(mesh.normals);
		this->texcoords = std::vector<glm::vec2>(mesh.texcoords);
		this->indices = std::vector<unsigned short>(mesh.indices);
		this->texname = std::string(mesh.texname);
		this->color = glm::vec3(mesh.color);
		this->AABBcalculated = mesh.AABBcalculated;
		this->AABBmin = mesh.AABBmin;
		this->AABBmax = mesh.AABBmax;
	}

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
		AABBcalculated = false;
		AABBmin = glm::vec3(1e300);
		AABBmax = glm::vec3(-1e300);
	}

	static Mesh meshFromShape(const tinyobj::shape_t &shape);

	glm::vec3 getAABBmin()
	{
		calculateAABB();
		return AABBmin;
	}

	glm::vec3 getAABBmax()
	{
		calculateAABB();
		return AABBmax;
	}

protected:
	glm::vec3 AABBmin;
	glm::vec3 AABBmax;
	bool AABBcalculated;

	void calculateAABB()
	{
		if (!AABBcalculated)
		{
			for (auto vertex : vertices)
			{
				AABBmin.x = glm::min(AABBmin.x, vertex.x);
				AABBmin.y = glm::min(AABBmin.y, vertex.y);
				AABBmin.z = glm::min(AABBmin.z, vertex.z);
				AABBmax.x = glm::max(AABBmax.x, vertex.x);
				AABBmax.y = glm::max(AABBmax.y, vertex.y);
				AABBmax.z = glm::max(AABBmax.z, vertex.z);
			}
			AABBcalculated = true;
		}
	}
};