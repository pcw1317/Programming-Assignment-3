#include "Mesh.h"
#include <algorithm>

using namespace glm;

Mesh Mesh::meshFromShape(const tinyobj::shape_t & shape)
{
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texcoords;
	std::vector<unsigned short> indices;
	std::string texname;
	glm::vec3 color;

	{
		int totalsize = shape.mesh.indices.size() / 3;
		int f = 0;
		while (f<totalsize)
		{
			int process = std::min(10000, totalsize - f);
			int point = 0;
			for (int i = f; i<process + f; i++)
			{
				int idx0 = shape.mesh.indices[3 * i];
				int idx1 = shape.mesh.indices[3 * i + 1];
				int idx2 = shape.mesh.indices[3 * i + 2];
				vec3 p0 = vec3(shape.mesh.positions[3 * idx0],
					shape.mesh.positions[3 * idx0 + 1],
					shape.mesh.positions[3 * idx0 + 2]);
				vec3 p1 = vec3(shape.mesh.positions[3 * idx1],
					shape.mesh.positions[3 * idx1 + 1],
					shape.mesh.positions[3 * idx1 + 2]);
				vec3 p2 = vec3(shape.mesh.positions[3 * idx2],
					shape.mesh.positions[3 * idx2 + 1],
					shape.mesh.positions[3 * idx2 + 2]);

				vertices.push_back(p0);
				vertices.push_back(p1);
				vertices.push_back(p2);

				if (shape.mesh.normals.size() > 0)
				{
					normals.push_back(vec3(shape.mesh.normals[3 * idx0],
						shape.mesh.normals[3 * idx0 + 1],
						shape.mesh.normals[3 * idx0 + 2]));
					normals.push_back(vec3(shape.mesh.normals[3 * idx1],
						shape.mesh.normals[3 * idx1 + 1],
						shape.mesh.normals[3 * idx1 + 2]));
					normals.push_back(vec3(shape.mesh.normals[3 * idx2],
						shape.mesh.normals[3 * idx2 + 1],
						shape.mesh.normals[3 * idx2 + 2]));
				}
				else
				{
					vec3 norm = normalize(glm::cross(normalize(p1 - p0), normalize(p2 - p0)));
					normals.push_back(norm);
					normals.push_back(norm);
					normals.push_back(norm);
				}

				if (shape.mesh.texcoords.size() > 0)
				{
					texcoords.push_back(vec2(shape.mesh.positions[2 * idx0],
						shape.mesh.positions[2 * idx0 + 1]));
					texcoords.push_back(vec2(shape.mesh.positions[2 * idx1],
						shape.mesh.positions[2 * idx1 + 1]));
					texcoords.push_back(vec2(shape.mesh.positions[2 * idx2],
						shape.mesh.positions[2 * idx2 + 1]));
				}
				else
				{
					vec2 tex(0.0);
					texcoords.push_back(tex);
					texcoords.push_back(tex);
					texcoords.push_back(tex);
				}
				indices.push_back(point++);
				indices.push_back(point++);
				indices.push_back(point++);
			}

			color = vec3(shape.material.diffuse[0],
				shape.material.diffuse[1],
				shape.material.diffuse[2]);
			texname = shape.material.name;//diffuse_texname;
			f = f + process;
		}
	}

	return Mesh(vertices, normals, texcoords, indices, texname, color);
}
