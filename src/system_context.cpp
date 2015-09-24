#include "system_context.h"
#include <sstream>
#include <iostream>
#include "host_mesh.h"
#include "device_mesh.h"

std::unique_ptr<system_context> system_context::global_context_ = nullptr;

void system_context::load_mesh(const char *path) {
    std::vector<tinyobj::shape_t> shapes;
    //read shapes using tinyobj first
    {
        std::string header, data, err;
        std::istringstream liness(path);

        getline(liness, header, '=');
        getline(liness, data, '=');
        if (strcmp(header.c_str(), "mesh") == 0) {
            std::size_t found = data.find_last_of("/\\");
            std::string path = data.substr(0, found + 1);
            std::cout << "Loading: " << data << std::endl;
            err = tinyobj::LoadObj(shapes, data.c_str(), path.c_str());
            if (!err.empty())
                throw std::runtime_error(err.c_str());
        }
    }

	//from tinyobj shapes, create mesh entries
    for (const auto &shape : shapes) {
        host_mesh_t mesh = host_mesh_t(shape);
        //std::cout << mesh.getAABBmax().x << " " << mesh.getAABBmax().y << " " << mesh.getAABBmax().z << ", " << mesh.getAABBmin().x << " " << mesh.getAABBmin().y <<  " " << mesh.getAABBmin().z << std::endl;
        drawMeshes.push_back(device_mesh_t(mesh));
        irKernel->addMesh(mesh);
        if (shape.material.name == "light") {
            // process light
			auto aabb = mesh.get_aabb();
			light.lightMin = aabb.first;
            light.lightMax = aabb.second;
            light.direction = glm::normalize(glm::vec3(0, -1, 0));
            light.intensity = glm::vec3(1, 1, 1) * 1e5f;
        }
    }
    irKernel->commitScene();
    auto vpls = irKernel->getVPLpos(light, 1, 0, 1);
    VPLs.insert(VPLs.end(), vpls.begin(), vpls.end());
}

