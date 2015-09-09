#pragma once

#include <glm/glm.hpp>
#include <string>

class Mesh;

namespace mesh_attributes
{
	enum
	{
		POSITION,
		NORMAL,
		TEXCOORD
	};
}

class DeviceMesh
{
public:
	unsigned int vertex_array;
	unsigned int vbo_indices;
	unsigned int num_indices;
	unsigned int vbo_vertices;
	unsigned int vbo_normals;
	unsigned int vbo_texcoords;
	glm::vec3 color;
	std::string texname;

public:
	DeviceMesh(unsigned int vao,
		unsigned int vbo_idx,
		unsigned int count_idx,
		unsigned int vbo_vtx,
		unsigned int vbo_norm,
		unsigned int vbo_uv,
		glm::vec3 color,
		std::string texName)
	{
		vertex_array = vao;
		vbo_indices = vbo_idx;
		num_indices = count_idx;
		vbo_vertices = vbo_vtx;
		vbo_normals = vbo_norm;
		vbo_texcoords = vbo_uv;
		this->color = color;
		texname = texName;
	}
	static DeviceMesh deviceMeshFromMesh(const Mesh &mesh);
};