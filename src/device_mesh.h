#pragma once

#include <glm/glm.hpp>
#include <string>

class host_mesh_t;

namespace mesh_attributes
{
	enum
	{
		POSITION,
		NORMAL,
		TEXCOORD
	};
}

class device_mesh_t
{
public:
	unsigned int vertex_array;
	unsigned int vbo_indices;
	unsigned int num_indices;
	unsigned int vbo_vertices;
	unsigned int vbo_normals;
	unsigned int vbo_texcoords;
	glm::vec3 ambient_color;
	glm::vec3 diffuse_color;
	std::string texture_name;

public:
	device_mesh_t(unsigned int vao,
		unsigned int vbo_idx,
		unsigned int count_idx,
		unsigned int vbo_vtx,
		unsigned int vbo_norm,
		unsigned int vbo_uv,
		glm::vec3 ambient_color,
		glm::vec3 diffuse_color,
		std::string texName)
	{
		vertex_array = vao;
		vbo_indices = vbo_idx;
		num_indices = count_idx;
		vbo_vertices = vbo_vtx;
		vbo_normals = vbo_norm;
		vbo_texcoords = vbo_uv;
		this->ambient_color = ambient_color;
		this->diffuse_color = diffuse_color;
		texture_name = texName;
	}
	static device_mesh_t deviceMeshFromMesh(const host_mesh_t &mesh);
};