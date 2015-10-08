#include "main.h"

#include <iostream>
#include <string>
#include <list>
#include <random>
#include <ctime>
#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/verbose_operator.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utility.h"
#include "system_context.h"
#include "device_mesh.h"
#include "gl_snippets.h"
#include "raytracer.h"

int mouse_buttons = 0;
int mouse_old_x = 0;
int mouse_old_y = 0;

system_context *context;

glm::mat4 get_global_mesh_world()
{
    return glm::mat4( 1.0 );
}

void update_title()
{
    //FPS calculation
    static unsigned long frame = 0;
    static double time_base = glfwGetTime();
    ++frame;
    double time_now = glfwGetTime();

    if( time_now - time_base > 1.0 ) //update title if a second passes
    {

        std::string title = utility::sprintfpp( "CS482 Instant Radiosity | FPS: %4.2f",
                                                frame / ( time_now - time_base )
                                              );

        glfwSetWindowTitle( context->window, title.c_str() );
        //reset per-second frame statistics for next update
        time_base = time_now;
        frame = 0;
    }
}

void perlight_draw( int light_index )
{
    glDisable( GL_BLEND );
    glEnable( GL_DEPTH_TEST );
    context->gls_programs[kGlsProgramSceneDraw].bind();
    context->gls_programs[kGlsProgramSceneDraw].set_uniforms(
        //"u_modelMat", "u_viewMat" , "u_perspMat", "u_vplPosition", "u_vplIntensity", "u_vplDirection", "u_numLights", "u_ambientColor", "u_diffuseColor", "u_shadowTex"
        gls::no_change,
        gls::no_change,
        gls::no_change,
        context->vpls[light_index].position,
        context->vpls[light_index].intensity,
        context->vpls[light_index].direction,
        gls::no_change,
        gls::no_change,
        gls::no_change,
        gls::no_change );
    context->gls_framebuffers[kGlsFramebufferSceneDraw].bind();
    context->gls_framebuffers[kGlsFramebufferSceneDraw].set_viewport();
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glActiveTexture( GL_TEXTURE0 );
    context->gls_cubemap_framebuffers[kGlsCubemapFramebufferShadow].get_color_map().bind();

    for( int i = 0; i < context->scene_meshes.size(); i++ )
    {
        context->gls_programs[kGlsProgramSceneDraw].set_uniforms(
            //"u_modelMat", "u_viewMat" , "u_perspMat", "u_vplPosition", "u_vplIntensity", "u_vplDirection", "u_numLights", "u_ambientColor", "u_diffuseColor", "u_shadowTex"
            gls::no_change,
            gls::no_change,
            gls::no_change,
            gls::no_change,
            gls::no_change,
            gls::no_change,
            gls::no_change,
            context->scene_meshes[i].ambient_color,
            context->scene_meshes[i].diffuse_color,
            gls::no_change );
        context->scene_meshes[i].draw();
    }
};

//from https://github.com/cforfang/opengl-shadowmapping/blob/master/src/vsmcube/main.cpp
std::pair<glm::mat4, glm::mat4> get_shadow_matrices( glm::vec3 light_pos, int dir )
{
    glm::mat4 v, p;
    p = glm::perspective( 90.0f, 1.0f, 0.1f, 1000.0f );
    switch( dir )
    {
    case 0:
        // +X
        v = glm::lookAt( light_pos, light_pos + glm::vec3( +1, +0, 0 ), glm::vec3( 0, -1, 0 ) );
        p *= v;
        break;
    case 1:
        // -X
        v = glm::lookAt( light_pos, light_pos + glm::vec3( -1, +0, 0 ), glm::vec3( 0, -1, 0 ) );
        p *= v;
        break;
    case 2:
        // +Y
        v = glm::lookAt( light_pos, light_pos + glm::vec3( 0, +1, 0 ), glm::vec3( 0, 0, -1 ) );
        p *= v;
        break;
    case 3:
        // -Y
        v = glm::lookAt( light_pos, light_pos + glm::vec3( 0, -1, 0 ), glm::vec3( 0, 0, -1 ) );
        p *= v;
        break;
    case 4:
        // +Z
        v = glm::lookAt( light_pos, light_pos + glm::vec3( 0, 0, +1 ), glm::vec3( 0, -1, 0 ) );
        p *= v;
        break;
    case 5:
        // -Z
        v = glm::lookAt( light_pos, light_pos + glm::vec3( 0, 0, -1 ), glm::vec3( 0, -1, 0 ) );
        p *= v;
        break;
    default:
        // Do nothing
        break;
    }
    return std::make_pair( v, p );
}

void perlight_generate_shadow_map( int light_index )
{
    glDisable( GL_BLEND );
    glEnable( GL_DEPTH_TEST );
    context->gls_programs[kGlsProgramShadowMapping].bind();
    context->gls_cubemap_framebuffers[kGlsCubemapFramebufferShadow].set_viewport();

    for( int i = 0; i < 6; ++i )
    {
        context->gls_cubemap_framebuffers[kGlsCubemapFramebufferShadow].bind( i );
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        auto vp = get_shadow_matrices( context->vpls[light_index].position, i );
        context->gls_programs[kGlsProgramShadowMapping].set_uniforms(
            //"u_model", "u_cameraToShadowView" , "u_cameraToShadowProjector"
            gls::no_change,
            vp.first,
            vp.second );
        for( int i = 0; i < context->scene_meshes.size(); i++ )
        {
            context->scene_meshes[i].draw();
        }
    }
};

void perlight_accumulate( int light_index )
{
    glEnable( GL_BLEND );
    glDisable( GL_DEPTH_TEST );
    glBlendFunc( GL_ONE, GL_ONE ); //perform additive blending

    context->gls_programs[kGlsProgramQuadDraw].bind();
    context->gls_framebuffers[kGlsFramebufferAccumulate].bind();
    context->gls_framebuffers[kGlsFramebufferAccumulate].set_viewport();
    glActiveTexture( GL_TEXTURE0 );
    context->gls_framebuffers[kGlsFramebufferSceneDraw].get_color_map().bind();
    context->quad_mesh.draw();
};

void visualize_accumulation()
{
    glDisable( GL_BLEND );
    glDisable( GL_DEPTH_TEST );

    context->gls_programs[kGlsProgramQuadDraw].bind();
    context->gls_framebuffers[kGlsFramebufferScreen].bind();
    context->gls_framebuffers[kGlsFramebufferScreen].set_viewport();
    glActiveTexture( GL_TEXTURE0 );
    context->gls_framebuffers[kGlsFramebufferAccumulate].get_color_map().bind();
    context->quad_mesh.draw();
};

void render()
{
    //clear accumulation framebuffer frst
    context->gls_framebuffers[kGlsFramebufferAccumulate].bind();
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glm::mat4 model = get_global_mesh_world(); //assume that model matrix is constant across models!
    glm::mat4 view = context->camera.get_view();
    glm::mat4 perspective = context->camera.get_perspective();

    //set global parameters
    context->gls_programs[kGlsProgramSceneDraw].bind();
    context->gls_programs[kGlsProgramSceneDraw].set_uniforms(
        //"u_modelMat", "u_viewMat" , "u_perspMat", "u_vplPosition", "u_vplIntensity", "u_vplDirection", "u_numLights", "u_ambientColor", "u_diffuseColor", "u_shadowTex"
        model,
        view,
        perspective,
        gls::no_change,
        gls::no_change,
        gls::no_change,
        context->shown_vpl_index == context->vpls.size() ? int( context->vpls.size() ) : 1,
        gls::no_change,
        gls::no_change,
        0 );
    context->gls_programs[kGlsProgramQuadDraw].bind();
    context->gls_programs[kGlsProgramQuadDraw].set_uniforms(
        //"u_Tex"
        0 );
    context->gls_programs[kGlsProgramShadowMapping].bind();
    context->gls_programs[kGlsProgramShadowMapping].set_uniforms(
        //"u_model", "u_cameraToShadowView" , "u_cameraToShadowProjector"
        model,
        gls::no_change,
        gls::no_change );

    if( context->shown_vpl_index == context->vpls.size() )
    {
        for( int light_index = 0; light_index < context->vpls.size(); ++light_index )
        {
            perlight_generate_shadow_map( light_index );
            perlight_draw( light_index );
            perlight_accumulate( light_index );
        }
    }
    else
    {
        perlight_generate_shadow_map( context->shown_vpl_index );
        perlight_draw( context->shown_vpl_index );
        perlight_accumulate( context->shown_vpl_index );
    }
    visualize_accumulation();
}

void window_callback_mouse_button( GLFWwindow *window, int button, int action, int mods )
{
    if( action == GLFW_PRESS )
    {
        mouse_buttons |= 1 << button;
    }
    else if( action == GLFW_RELEASE )
    {
        mouse_buttons = 0;
    }
    {
        double x, y;
        glfwGetCursorPos( window, &x, &y );

        mouse_old_x = int( x );
        mouse_old_y = int( y );
    }
}

void window_callback_cursor_pos( GLFWwindow *window, double x, double y )
{
    float dx, dy;
    dx = -( float )( x - mouse_old_x );
    dy = ( float )( y - mouse_old_y );
    float sensitivity = 0.001f;

    if( mouse_buttons & 1 << GLFW_MOUSE_BUTTON_LEFT )
    {
        context->camera.rotate( glm::vec3( dy * sensitivity, 0, dx * sensitivity ) );
    }

    mouse_old_x = int( x );
    mouse_old_y = int( y );
}

void window_callback_key( GLFWwindow *window, int key, int scancode, int action, int mods )
{
    float tx = 0;
    float ty = 0;
    float tz = 0;
    if( action == GLFW_RELEASE ) //no need to process key up events
        return;
    float speed = 10.f;
    switch( key )
    {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose( window, true );
        break;
    case( 'W' ) :
        tz = speed;
        break;
    case( 'S' ) :
        tz = -speed;
        break;
    case( 'D' ) :
        tx = -speed;
        break;
    case( 'A' ) :
        tx = speed;
        break;
    case( 'Q' ) :
        ty = speed;
        break;
    case( 'Z' ) :
        ty = -speed;
        break;
    case( ' ' ):
        context->shown_vpl_index = ( context->shown_vpl_index + 1 ) % ( context->vpls.size() + 1 );
        break;
    }

    if( abs( tx ) > 0 ||  abs( tz ) > 0 || abs( ty ) > 0 )
    {
        context->camera.translate( glm::vec3( tx, ty, tz ) );
    }
}

device_mesh_t init_quad_mesh()
{
    std::vector<glm::vec3> positions
    {
        glm::vec3( -1, 1, 0 ),
        glm::vec3( -1, -1, 0 ),
        glm::vec3( 1, -1, 0 ),
        glm::vec3( 1, 1, 0 )
    };

    std::vector<glm::vec3> normals
    {
        glm::vec3( 0, 0, 0 ),
        glm::vec3( 0, 0, 0 ),
        glm::vec3( 0, 0, 0 ),
        glm::vec3( 0, 0, 0 )
    };

    std::vector<glm::vec2> texcoords
    {
        glm::vec2( 0, 1 ),
        glm::vec2( 0, 0 ),
        glm::vec2( 1, 0 ),
        glm::vec2( 1, 1 )
    };

    std::vector<unsigned short> indices{ 0, 1, 2, 0, 2, 3 };

    host_mesh_t hm( positions, normals, texcoords, indices, "", glm::vec3( 0, 0, 0 ), glm::vec3( 0, 0, 0 ) );
    return device_mesh_t( hm );
}

void init()
{
    //GL parameter initialization
    glEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

    //context gls object initialization
    context->gls_programs.resize( kGlsProgramMax );
    context->gls_buffers.resize( kGlsBufferMax );
    context->gls_vertex_arrays.resize( kGlsVertexArrayMax );
    context->gls_framebuffers.resize( kGlsFramebufferMax );
    context->gls_cubemap_framebuffers.resize( kGlsCubemapFramebufferMax );

    //shaders
    context->gls_programs[kGlsProgramSceneDraw] = gls::program( kProgramSceneDraw );
    context->gls_programs[kGlsProgramQuadDraw] = gls::program( kProgramQuadDraw );
    context->gls_programs[kGlsProgramShadowMapping] = gls::program( kProgramShadowMapping );

    //framebuffers
    context->gls_framebuffers[kGlsFramebufferScreen] = gls::framebuffer<gls::texture, gls::texture>( context->viewport.x, context->viewport.y, true ); //default screen framebuffer
    context->gls_framebuffers[kGlsFramebufferSceneDraw] = gls::framebuffer<gls::texture, gls::texture>( context->viewport.x, context->viewport.y );
    context->gls_framebuffers[kGlsFramebufferAccumulate] = gls::framebuffer<gls::texture, gls::texture>( context->viewport.x, context->viewport.y );

    //cubemap framebuffers
    context->gls_cubemap_framebuffers[kGlsCubemapFramebufferShadow] = gls::cubemap_framebuffer<gls::texture, gls::texture>( kShadowSize ); //default screen framebuffer

    //quad geometry; used for various texture-to-texture operations
    context->quad_mesh = std::move( init_quad_mesh() );
}

namespace
{
//opengl initialization: GLFW, GLEW and our application window
class opengl_initializer_t
{
public:
    opengl_initializer_t();
    opengl_initializer_t( const opengl_initializer_t & ) = delete;
    opengl_initializer_t &operator=( const opengl_initializer_t & ) = delete;
    ~opengl_initializer_t();
    opengl_initializer_t( opengl_initializer_t && ) = delete;
    opengl_initializer_t &operator=( opengl_initializer_t && ) = delete;
};

opengl_initializer_t::opengl_initializer_t()
{
    //initialize glfw
    if( !glfwInit() )
        throw std::runtime_error( "glfwInit() failed" );

    try
    {
        //create window
        glfwWindowHint( GLFW_RESIZABLE, GL_FALSE );
        if( !( context->window = glfwCreateWindow( context->viewport.x, context->viewport.y, "InstantRadiosity", NULL, NULL ) ) )
            throw std::runtime_error( "glfw window creation failed" );


        glfwMakeContextCurrent( context->window );

        //set callbacks
        glfwSetKeyCallback( context->window, window_callback_key );
        glfwSetCursorPosCallback( context->window, window_callback_cursor_pos );
        glfwSetMouseButtonCallback( context->window, window_callback_mouse_button );

        //initialize glew
        if( glewInit() != GLEW_OK )
            throw std::runtime_error( "glewInit() failed" );

        //check version requirement
        //TODO: update correct requirement later
        if( !GLEW_VERSION_3_3 )
            throw std::runtime_error( "This program requires OpenGL 3.3 class graphics card." );
        else
        {
            std::cerr << "Status: Using GLEW " << glewGetString( GLEW_VERSION ) << std::endl;
            std::cerr << "OpenGL version " << glGetString( GL_VERSION ) << " supported" << std::endl;
        }
    }
    catch( ... )
    {
        glfwTerminate();
        throw;
    }
}
opengl_initializer_t::~opengl_initializer_t()
{
    glfwTerminate();
}
}
int main( int argc, char *argv[] )
{
    //Step 0: Initialize our system context
    {
        glm::uvec2 viewport( 1280, 720 );
        camera_t default_camera(
            glm::vec3( 300, 300, -500 ),
            glm::vec3( 0, 0, 1 ),
            glm::vec3( 0, 1, 0 ),
            glm::perspective(
                45.0f,
                float( viewport.x ) / float( viewport.y ),
                kNearPlane,
                kFarPlane
            )
        );
        context = system_context::initialize( default_camera, viewport );
    }

    //Step 1: Initialize GLFW & GLEW
    //RAII initialization of GLFW, GLEW and our application window
    std::unique_ptr<opengl_initializer_t> opengl_initializer;
    try
    {
        opengl_initializer.reset( new opengl_initializer_t );
    }
    catch( const std::exception &e )
    {
        std::cerr << "OpenGL initialization failed. Reason: " << e.what() << "\nAborting.\n";
        return EXIT_FAILURE;
    }

    //Step 2: Load mesh into memory
    if( argc > 1 )
    {
        try
        {
            context->load_mesh( argv[1] );
        }
        catch( const std::exception &e )
        {
            std::cerr << "Mesh load failed. Reason: " << e.what() << "\nAborting.\n";
            return EXIT_FAILURE;
        }
    }
    else
    {
        std::cerr << utility::sprintfpp( "Usage: %s mesh=[obj file]\n", argv[0] );
        return EXIT_SUCCESS;
    }

    //Step 3: Initialize objects
    try
    {
        init();
    }
    catch( const std::exception &e )
    {
        std::cerr << "Object initialization failed. Reason: " << e.what() << "\nAborting.\n";
        return EXIT_FAILURE;
    }

    //Step 4: Main loop
    while( !glfwWindowShouldClose( context->window ) )
    {
        render();
        update_title();

        glfwSwapBuffers( context->window );
        glfwPollEvents();
    }

    return EXIT_SUCCESS;
}