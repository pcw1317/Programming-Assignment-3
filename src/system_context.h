#ifndef _SYSTEM_CONTEXT_H_
#define _SYSTEM_CONTEXT_H_

#include <vector>
#include <cassert>
#include <memory>
#include <exception>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <tiny_obj_loader/tiny_obj_loader.h>

#include "gl_snippets.h"
#include "camera.h"
#include "raytracer.h"
#include "device_mesh.h"

class system_context
{
public:
    ~system_context() {}
    system_context( const system_context & ) = delete;
    system_context &operator=( const system_context & ) = delete;
    system_context( system_context && ) = delete;
    system_context &operator=( system_context && ) = delete;

    void load_mesh( const char *path );

protected:
    system_context( const camera_t &camera, const glm::uvec2 &viewport ) :
        camera( camera ), viewport( viewport ), vpl_raytracer( new raytracer() )
    {
    }
    std::unique_ptr<raytracer> vpl_raytracer;
    area_light_t light;

public:
    std::vector<device_mesh_t> scene_meshes;
    device_mesh_t quad_mesh;
    GLFWwindow *window;
    camera_t camera;
    glm::uvec2 viewport;
    std::vector<point_light_t> vpls;

    //gls objects
    std::vector<gls::program> gls_programs;
    std::vector<gls::buffer> gls_buffers;
    std::vector<gls::vertex_array> gls_vertex_arrays;
    std::vector<gls::framebuffer<gls::texture, gls::texture> > gls_framebuffers;
    std::vector<gls::cubemap_framebuffer<gls::texture, gls::texture> > gls_cubemap_framebuffers;

    void initialize_quad_mesh();

    unsigned int shown_vpl_index = 0;

protected:
    static std::unique_ptr<system_context> global_context_;
public:
    template<typename... Args>
    static system_context *initialize( Args &&... args )
    {
        assert( global_context_.get() == nullptr );
        global_context_.reset( new system_context( std::forward<Args>( args )... ) );
        return global_context_.get();
    }
    static system_context *get()
    {
        assert( global_context_.get() != nullptr );
        return global_context_.get();
    }
};

#endif
