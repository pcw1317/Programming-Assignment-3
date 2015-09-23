#ifndef _MESH_H_
#define _MESH_H_

#include <vector>
#include <algorithm>
#include <glm/glm.hpp>
#include <tiny_obj_loader/tiny_obj_loader.h>

class host_mesh_t {
  public:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<unsigned short> indices;
    std::string texture_name;
    glm::vec3 ambient_color;
    glm::vec3 diffuse_color;

  public:
    host_mesh_t() {
        AABBcalculated = false;
        AABBmin = glm::vec3(1e300);
        AABBmax = glm::vec3(-1e300);
    }

    host_mesh_t(host_mesh_t &mesh) {
        this->vertices = std::vector<glm::vec3>(mesh.vertices);
        this->normals = std::vector<glm::vec3>(mesh.normals);
        this->texcoords = std::vector<glm::vec2>(mesh.texcoords);
        this->indices = std::vector<unsigned short>(mesh.indices);
        this->texture_name = std::string(mesh.texture_name);
        this->diffuse_color = glm::vec3(mesh.diffuse_color);
        this->AABBcalculated = mesh.AABBcalculated;
        this->AABBmin = mesh.AABBmin;
        this->AABBmax = mesh.AABBmax;
    }

    host_mesh_t(std::vector<glm::vec3> &vertices,
                std::vector<glm::vec3> &normals,
                std::vector<glm::vec2> &texcoords,
                std::vector<unsigned short> &indices,
                std::string &texture_name,
                glm::vec3 &ambient_color,
                glm::vec3 &diffuse_color) {
        this->vertices = std::vector<glm::vec3>(vertices);
        this->normals = std::vector<glm::vec3>(normals);
        this->texcoords = std::vector<glm::vec2>(texcoords);
        this->indices = std::vector<unsigned short>(indices);
        this->texture_name = std::string(texture_name);
        this->ambient_color = glm::vec3(ambient_color);
        this->diffuse_color = glm::vec3(diffuse_color);
        AABBcalculated = false;
        AABBmin = glm::vec3(1e300);
        AABBmax = glm::vec3(-1e300);
    }

    explicit host_mesh_t(const tinyobj::shape_t &shape);

    glm::vec3 getAABBmin() {
        calculateAABB();
        return AABBmin;
    }

    glm::vec3 getAABBmax() {
        calculateAABB();
        return AABBmax;
    }

  protected:
    glm::vec3 AABBmin;
    glm::vec3 AABBmax;
    bool AABBcalculated;

    void calculateAABB() {
        if(!AABBcalculated) {
            for(auto vertex : vertices) {
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

#endif