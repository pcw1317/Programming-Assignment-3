#include "system_context.h"

#include <sstream>
#include <iostream>
#include <vector>
#include "host_mesh.h"
#include "device_mesh.h"
#include "main.h"

std::unique_ptr<system_context> system_context::global_context_ = nullptr;

void system_context::initialize_quad_mesh() {
	std::vector<glm::vec3> positions{
		glm::vec3(-1, 1, 0),
		glm::vec3(-1, -1, 0),
		glm::vec3(1, -1, 0),
		glm::vec3(1, 1, 0)
	};

	std::vector<glm::vec3> normals{
		glm::vec3(0, 0, 0),
		glm::vec3(0, 0, 0),
		glm::vec3(0, 0, 0),
		glm::vec3(0, 0, 0)
	};

	std::vector<glm::vec2> texcoords{
		glm::vec2(0, 1),
		glm::vec2(0, 0),
		glm::vec2(1, 0),
		glm::vec2(1, 1)
	};

	std::vector<unsigned short> indices{ 0, 1, 2, 0, 2, 3 };

	host_mesh_t hm(positions, normals, texcoords, indices, "", glm::vec3(0, 0, 0), glm::vec3(0, 0, 0));
	quad_mesh.reset(new device_mesh_t(hm));
}

void system_context::load_mesh( const char *path )
{
    std::vector<tinyobj::shape_t> shapes;
    //read shapes using tinyobj first
    {
        std::string header, data, err;
        std::istringstream liness( path );

        getline( liness, header, '=' );
        getline( liness, data, '=' );
        if( header == "mesh" )
        {
            std::size_t found = data.find_last_of( "/\\" );
            std::string path = data.substr( 0, found + 1 );
            std::cout << "Loading: " << data << std::endl;
            err = tinyobj::LoadObj( shapes, data.c_str(), path.c_str() );
            if( !err.empty() )
                throw std::runtime_error( err.c_str() );
        }
    }

    //from tinyobj shapes, create mesh entries
    for( const auto &shape : shapes )
    {
        host_mesh_t mesh = host_mesh_t( shape );
        //std::cout << mesh.get_aabb().first.x << " " << mesh.get_aabb().first.y << " " << mesh.get_aabb().first.z << ", " << mesh.get_aabb().second.x << " " << mesh.get_aabb().second.y <<  " " << mesh.get_aabb().second.z << std::endl;
        drawMeshes.push_back( device_mesh_t( mesh ) );
        irKernel->add_mesh( mesh );
        if( shape.material.name == "light" )
        {
            // process light; use ambient color as intensity
            auto aabb = mesh.get_aabb();
            light.aabb_min = aabb.first;
            light.aabb_max = aabb.second;
            light.direction = glm::normalize( glm::vec3( 0, -1, 0 ) );
			light.intensity = glm::make_vec3(shape.material.ambient);
        }
    }
    irKernel->commit_scene();
     auto vpls = irKernel->compute_vpl( light, kVplCount);
    VPLs.insert( VPLs.end(), vpls.begin(), vpls.end() );
}

