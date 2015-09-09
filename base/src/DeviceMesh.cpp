#include "DeviceMesh.h"
#include "Mesh.h"
#include <GL/glew.h>

DeviceMesh DeviceMesh::deviceMeshFromMesh(const Mesh & mesh)
{
	unsigned int vertex_array;
	unsigned int vbo_indices;
	unsigned int num_indices;
	unsigned int vbo_vertices;
	unsigned int vbo_normals;
	unsigned int vbo_texcoords;
	glm::vec3 color;
	std::string texname;

	//Allocate vertex array
	//Vertex arrays encapsulate a set of generic vertex 
	//attributes and the buffers they are bound to
	//Different vertex array per mesh.
	glGenVertexArrays(1, &vertex_array);
	glBindVertexArray(vertex_array);

	//Allocate vbos for data
	glGenBuffers(1, &vbo_vertices);
	glGenBuffers(1, &vbo_normals);
	glGenBuffers(1, &vbo_indices);
	glGenBuffers(1, &vbo_texcoords);

	//Upload vertex data
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
	glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size()*sizeof(glm::vec3),
		&mesh.vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(mesh_attributes::POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(mesh_attributes::POSITION);

	//Upload normal data
	glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
	glBufferData(GL_ARRAY_BUFFER, mesh.normals.size()*sizeof(glm::vec3),
		&mesh.normals[0], GL_STATIC_DRAW);
	glVertexAttribPointer(mesh_attributes::NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(mesh_attributes::NORMAL);

	//Upload texture coord data
	glBindBuffer(GL_ARRAY_BUFFER, vbo_texcoords);
	glBufferData(GL_ARRAY_BUFFER, mesh.texcoords.size()*sizeof(glm::vec2),
		&mesh.texcoords[0], GL_STATIC_DRAW);
	glVertexAttribPointer(mesh_attributes::TEXCOORD, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(mesh_attributes::TEXCOORD);

	//indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size()*sizeof(GLushort),
		&mesh.indices[0], GL_STATIC_DRAW);
	num_indices = mesh.indices.size();
	//Unplug Vertex Array
	glBindVertexArray(0);

	texname = mesh.texname;
	color = mesh.color;

	return DeviceMesh(vertex_array, vbo_indices, num_indices, vbo_vertices, vbo_normals, vbo_texcoords, color, texname);
}
