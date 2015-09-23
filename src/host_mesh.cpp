#include "host_mesh.h"
#include <algorithm>

host_mesh_t::host_mesh_t(const tinyobj::shape_t &shape) {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<unsigned short> indices;
    std::string texture_name;
    glm::vec3 ambient_color;
    glm::vec3 diffuse_color;

    {
        int totalsize = shape.mesh.indices.size() / 3;
        int f = 0;
        while(f < totalsize) {
            int process = std::min(10000, totalsize - f);
            int point = 0;
            for(int i = f; i < process + f; i++) {
                int idx0 = shape.mesh.indices[3 * i];
                int idx1 = shape.mesh.indices[3 * i + 1];
                int idx2 = shape.mesh.indices[3 * i + 2];
                glm::vec3 p0 = glm::vec3(shape.mesh.positions[3 * idx0],
                                         shape.mesh.positions[3 * idx0 + 1],
                                         shape.mesh.positions[3 * idx0 + 2]);
                glm::vec3 p1 = glm::vec3(shape.mesh.positions[3 * idx1],
                                         shape.mesh.positions[3 * idx1 + 1],
                                         shape.mesh.positions[3 * idx1 + 2]);
                glm::vec3 p2 = glm::vec3(shape.mesh.positions[3 * idx2],
                                         shape.mesh.positions[3 * idx2 + 1],
                                         shape.mesh.positions[3 * idx2 + 2]);

                vertices.push_back(p0);
                vertices.push_back(p1);
                vertices.push_back(p2);

                if(shape.mesh.normals.size() > 0) {
                    normals.push_back(glm::vec3(shape.mesh.normals[3 * idx0],
                                                shape.mesh.normals[3 * idx0 + 1],
                                                shape.mesh.normals[3 * idx0 + 2]));
                    normals.push_back(glm::vec3(shape.mesh.normals[3 * idx1],
                                                shape.mesh.normals[3 * idx1 + 1],
                                                shape.mesh.normals[3 * idx1 + 2]));
                    normals.push_back(glm::vec3(shape.mesh.normals[3 * idx2],
                                                shape.mesh.normals[3 * idx2 + 1],
                                                shape.mesh.normals[3 * idx2 + 2]));
                } else {
                    glm::vec3 norm = glm::normalize(glm::cross(glm::normalize(p1 - p0), glm::normalize(p2 - p0)));
                    normals.push_back(norm);
                    normals.push_back(norm);
                    normals.push_back(norm);
                }

                if(shape.mesh.texcoords.size() > 0) {
                    texcoords.push_back(glm::vec2(shape.mesh.positions[2 * idx0],
                                                  shape.mesh.positions[2 * idx0 + 1]));
                    texcoords.push_back(glm::vec2(shape.mesh.positions[2 * idx1],
                                                  shape.mesh.positions[2 * idx1 + 1]));
                    texcoords.push_back(glm::vec2(shape.mesh.positions[2 * idx2],
                                                  shape.mesh.positions[2 * idx2 + 1]));
                } else {
                    glm::vec2 tex(0.0);
                    texcoords.push_back(tex);
                    texcoords.push_back(tex);
                    texcoords.push_back(tex);
                }
                indices.push_back(point++);
                indices.push_back(point++);
                indices.push_back(point++);
            }

            ambient_color = glm::vec3(shape.material.ambient[0],
                                      shape.material.ambient[1],
                                      shape.material.ambient[2]);
            diffuse_color = glm::vec3(shape.material.diffuse[0],
                                      shape.material.diffuse[1],
                                      shape.material.diffuse[2]);
            texture_name = shape.material.name;
            f = f + process;
        }
    }

    *this = std::move(host_mesh_t(vertices, normals, texcoords, indices, texture_name, ambient_color, diffuse_color));
}
