#include "main.h"

#include "Utility.h"
#include "Light.h"
#include "system_context.h"
#include "device_mesh.h"
#include "gl_snippets.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/verbose_operator.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <list>
#include <random>
#include <ctime>
#include <algorithm>

#include "InstantRadiosity.h"

int mouse_buttons = 0;
int mouse_old_x = 0, mouse_dof_x = 0;
int mouse_old_y = 0, mouse_dof_y = 0;

system_context *context;

glm::mat4 get_mesh_world();

device_mesh2_t device_quad;

void init_quad() {
    vertex2_t verts[] = { {glm::vec3(-1, 1, 0), glm::vec2(0, 1)},
        {glm::vec3(-1, -1, 0), glm::vec2(0, 0)},
        {glm::vec3(1, -1, 0), glm::vec2(1, 0)},
        {glm::vec3(1, 1, 0), glm::vec2(1, 1)}
    };

    unsigned short indices[] = { 0, 1, 2, 0, 2, 3 };

    //Allocate vertex array
    //Vertex arrays encapsulate a set of generic vertex attributes and the buffers they are bound too
    //Different vertex array per mesh.
    context->gls_vertex_arrays[kGlsVertexArrayQuad] = gls::vertex_array();
    context->gls_vertex_arrays[kGlsVertexArrayQuad].bind();

    //Allocate vbos for data and indices
    context->gls_buffers[kGlsBufferQuadVertexBuffer] = gls::buffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    context->gls_buffers[kGlsBufferQuadIndexBuffer] = gls::buffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);

    //Upload vertex data
    context->gls_buffers[kGlsBufferQuadVertexBuffer].bind();
    context->gls_buffers[kGlsBufferQuadVertexBuffer].set_data(verts, std::size(verts), sizeof(vertex2_t));

    //Use of strided data, Array of Structures instead of Structures of Arrays
    context->gls_vertex_arrays[kGlsVertexArrayQuad].set_attribute(quad_attributes::POSITION, context->gls_buffers[kGlsBufferQuadVertexBuffer], 3, GL_FLOAT, false, sizeof(vertex2_t), 0);
    context->gls_vertex_arrays[kGlsVertexArrayQuad].set_attribute(quad_attributes::TEXCOORD, context->gls_buffers[kGlsBufferQuadVertexBuffer], 2, GL_FLOAT, false, sizeof(vertex2_t), sizeof(glm::vec3));

    //Upload index data
    context->gls_buffers[kGlsBufferQuadIndexBuffer].bind();
    context->gls_buffers[kGlsBufferQuadVertexBuffer].set_data(indices, std::size(indices), sizeof(unsigned short));

    //Unplug Vertex Array
    context->gls_vertex_arrays[kGlsVertexArrayQuad].unbind();
}

glm::mat4 get_mesh_world() {
    return glm::mat4(1.0);
}

Display display_type = DISPLAY_TOTAL;
void draw_quad() {
    glBindVertexArray(device_quad.vertex_array);
    context->gls_buffers[kGlsBufferQuadIndexBuffer].bind();
    glDrawElements(GL_TRIANGLES, GLsizei(context->gls_buffers[kGlsBufferQuadIndexBuffer].num_elements()), GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
}

void update_title() {
    //FPS calculation
    static unsigned long frame = 0;
    static double time_base = glfwGetTime();
    ++frame;
    double time_now = glfwGetTime();

    if(time_now - time_base > 1.0) { //update title if a second passes
        const char *displaying;
        switch(display_type) {
        case(DISPLAY_DEPTH) :
            displaying = "Depth";
            break;
        case(DISPLAY_NORMAL) :
            displaying = "Normal";
            break;
        case(DISPLAY_COLOR) :
            displaying = "Color";
            break;
        case(DISPLAY_POSITION) :
            displaying = "Position";
            break;
        case(DISPLAY_TOTAL) :
            displaying = "Diffuse";
            break;
        case(DISPLAY_LIGHTS) :
            displaying = "Lights";
            break;
        case DISPLAY_GLOWMASK:
            displaying = "Glow Mask";
            break;
        case(DISPLAY_SHADOW) :
            displaying = "ShadowMap";
            break;
        }

        std::string title = utility::sprintfpp("CS482 Instant Radiosity | Displaying <%s> | FPS: %4.2f",
                                               displaying,
                                               frame / (time_now - time_base)
                                              );

        glfwSetWindowTitle(context->window, title.c_str());
        //reset per-second frame statistics for next update
        time_base = time_now;
        frame = 0;
    }
}

void render_forward() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glm::mat4 model = get_mesh_world();
    glm::mat4 view = context->pCam.get_view(); // camera_t view Matrix
    glm::mat4 perspective = context->pCam.get_perspective();

    context->gls_programs[kGlsProgramSceneDraw].bind();
    context->gls_programs[kGlsProgramSceneDraw].set_uniform<int>(6 /*u_numLights*/, int(context->VPLs.size()));
    /*
    if (indirectON)
    glUniform1i(glGetUniformLocation(forward_shading_prog, "u_numVPLs"), nVPLs);
    else
    glUniform1i(glGetUniformLocation(forward_shading_prog, "u_numVPLs"), 0);
    */
    {
        context->gls_programs[kGlsProgramSceneDraw].set_uniforms_from(0 /*u_ModelMat*/, model, view, perspective);
        context->gls_framebuffers[kGlsFramebufferAccumulate].bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for(int lightIter = 0; lightIter < context->VPLs.size(); ++lightIter) {
            context->gls_programs[kGlsProgramSceneDraw].set_uniforms_from(3 /*u_vplPosition*/, context->VPLs[lightIter].position, context->VPLs[lightIter].intensity, context->VPLs[lightIter].direction);

            context->gls_programs[kGlsProgramSceneDraw].bind();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            for(int i = 0; i < context->drawMeshes.size(); i++) {
                context->gls_programs[kGlsProgramSceneDraw].set_uniforms_from(7 /*u_AmbientColor*/, context->drawMeshes[i].ambient_color, context->drawMeshes[i].diffuse_color);
                glBindVertexArray(context->drawMeshes[i].vertex_array);
                glDrawElements(GL_TRIANGLES, context->drawMeshes[i].num_indices, GL_UNSIGNED_SHORT, 0);
            }

            /*
            glUseProgram(progs[kGlsProgramQuadDraw]);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            // bind accumulation texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[kGlsTextureScene]);
            glUniform1i(textureLoc, 0);
            // draw screenquad with accumulation texture
            glBindVertexArray(device_quad.vertex_array);
            glDrawElements(GL_TRIANGLES, device_quad.num_indices, GL_UNSIGNED_SHORT, 0);
            */
            /*
            glUseProgram(progs[kGlsProgramQuadDraw]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo[kGlsFramebufferAccumulate]);
            // bind scenedraw texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures[kGlsTextureScene]);
            glUniform1i(textureLoc, 0);
            // enable blend
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            // draw screenquad with scenedraw texture
            glBindVertexArray(device_quad.vertex_array);
            glDrawElements(GL_TRIANGLES, device_quad.num_indices, GL_UNSIGNED_SHORT, 0);
            // disable blend
            glDisable(GL_BLEND);
            */
        }
        /*
        glUseProgram(progs[kGlsProgramQuadDraw]);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // bind accumulation texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[kGlsTextureAccumulate]);
        glUniform1i(textureLoc, 0);
        // draw screenquad with accumulation texture
        glBindVertexArray(device_quad.vertex_array);
        glDrawElements(GL_TRIANGLES, device_quad.num_indices, GL_UNSIGNED_SHORT, 0);
        */
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void window_callback_mouse_button(GLFWwindow *window, int button, int action, int mods) {
    if(action == GLFW_PRESS) {
        mouse_buttons |= 1 << button;
    } else if(action == GLFW_RELEASE) {
        mouse_buttons = 0;
    }
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        mouse_old_x = int(x);
        mouse_old_y = int(y);
    }
    if(button == GLFW_MOUSE_BUTTON_RIGHT) {
        mouse_dof_x = mouse_old_x;
        mouse_dof_y = mouse_old_y;
    }
}

void window_callback_cursor_pos(GLFWwindow *window, double x, double y) {
    float dx, dy;
    dx = -(float)(x - mouse_old_x);
    dy = (float)(y - mouse_old_y);
    float sensitivity = 0.001f;

    if(mouse_buttons & 1 << GLFW_MOUSE_BUTTON_RIGHT) {
        //context->pCam.adjust(0,0,dx,0,0,0);;
    } else if(mouse_buttons & 1 << GLFW_MOUSE_BUTTON_LEFT) {
        context->pCam.rotate(glm::vec3(dy * sensitivity, 0, dx * sensitivity));
    }

    mouse_old_x = int(x);
    mouse_old_y = int(y);
}

void window_callback_key(GLFWwindow *window, int key, int scancode, int action, int mods) {
    float tx = 0;
    float ty = 0;
    float tz = 0;
    if(action == GLFW_RELEASE)  //no need to process key up events
        return;
    float speed = 10.f;
    switch(key) {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, true);
        break;
    case('W') :
        tz = speed;
        break;
    case('S') :
        tz = -speed;
        break;
    case('D') :
        tx = -speed;
        break;
    case('A') :
        tx = speed;
        break;
    case('Q') :
        ty = speed;
        break;
    case('Z') :
        ty = -speed;
        break;
    case('1'):
        display_type = DISPLAY_DEPTH;
        break;
    case('2'):
        display_type = DISPLAY_NORMAL;
        break;
    case('3'):
        display_type = DISPLAY_COLOR;
        break;
    case('4'):
        display_type = DISPLAY_POSITION;
        break;
    case('5'):
        display_type = DISPLAY_LIGHTS;
        break;
    case('6'):
        display_type = DISPLAY_GLOWMASK;
        break;
    case('0'):
        display_type = DISPLAY_TOTAL;
        break;
    case('7'):
        display_type = DISPLAY_SHADOW;
        break;
    case(' '):
        context->shown_vpl_index = (context->shown_vpl_index + 1) % context->VPLs.size();
        break;
    }

    if(abs(tx) > 0 ||  abs(tz) > 0 || abs(ty) > 0) {
        context->pCam.translate(glm::vec3(tx, ty, tz));
    }
}

void init() {
    //GL parameter initialization
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    //context gls object initialization
    context->gls_programs.resize(kGlsProgramMax);
    context->gls_buffers.resize(kGlsBufferMax);
    context->gls_vertex_arrays.resize(kGlsVertexArrayMax);
    context->gls_framebuffers.resize(kGlsFramebufferMax);

    //shaders
    context->gls_programs[kGlsProgramSceneDraw] = gls::program(kProgramSceneDraw);
    context->gls_programs[kGlsProgramQuadDraw] = gls::program(kProgramQuadDraw);

    //framebuffers
    context->gls_framebuffers[kGlsFramebufferSceneDraw] = gls::framebuffer<gls::texture, gls::texture>(context->viewport.x, context->viewport.y);
    context->gls_framebuffers[kGlsFramebufferAccumulate] = gls::framebuffer<gls::texture, gls::texture>(context->viewport.x, context->viewport.y);
    context->gls_framebuffers[kGlsFramebufferAccumulate].unbind();
}

namespace {
//opengl initialization: GLFW, GLEW and our application window
class opengl_initializer_t {
  public:
    opengl_initializer_t();
    opengl_initializer_t(const opengl_initializer_t &) = delete;
    opengl_initializer_t &operator=(const opengl_initializer_t &) = delete;
    ~opengl_initializer_t();
    opengl_initializer_t(opengl_initializer_t &&) = delete;
    opengl_initializer_t &operator=(opengl_initializer_t &&) = delete;
};

opengl_initializer_t::opengl_initializer_t() {
    //initialize glfw
    if(!glfwInit())
        throw std::runtime_error("glfwInit() failed");

    try {
        //create window
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        if(!(context->window = glfwCreateWindow(context->viewport.x, context->viewport.y, "InstantRadiosity", NULL, NULL)))
            throw std::runtime_error("glfw window creation failed");
        glfwMakeContextCurrent(context->window);

        //set callbacks
        glfwSetKeyCallback(context->window, window_callback_key);
        glfwSetCursorPosCallback(context->window, window_callback_cursor_pos);
        glfwSetMouseButtonCallback(context->window, window_callback_mouse_button);

        //initialize glew
        if(glewInit() != GLEW_OK)
            throw std::runtime_error("glewInit() failed");

        //check version requirement
        //TODO: update correct requirement later
        if(!GLEW_VERSION_3_3 || !GLEW_ARB_compute_shader)
            throw std::runtime_error("This program requires OpenGL 3.3 class graphics card.");
        else {
            std::cerr << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;
            std::cerr << "OpenGL version " << glGetString(GL_VERSION) << " supported" << std::endl;
        }
    } catch(...) {
        glfwTerminate();
        throw;
    }
}
opengl_initializer_t::~opengl_initializer_t() {
    glfwTerminate();
}
}
int main(int argc, char *argv[]) {
    //Step 0: Initialize our system context
    {
        glm::uvec2 viewport(1280, 720);
        camera_t default_camera(
            glm::vec3(300, 300, -500),
            glm::vec3(0, 0, 1),
            glm::vec3(0, 1, 0),
            glm::perspective(
                45.0f,
                float(viewport.x) / float(viewport.y),
                kNearPlane,
                kFarPlane
            )
        );
        context = system_context::initialize(default_camera, viewport);
    }

    //Step 1: Initialize GLFW & GLEW
    //RAII initialization of GLFW, GLEW and our application window
    std::unique_ptr<opengl_initializer_t> opengl_initializer;
    try {
        opengl_initializer.reset(new opengl_initializer_t);
    } catch(const std::exception &e) {
        std::cerr << "OpenGL initialization failed. Reason: " << e.what() << "\nAborting.\n";
        return EXIT_FAILURE;
    }

    //Step 2: Load mesh into memory
    if(argc > 1) {
        try {
            context->load_mesh(argv[1]);
        } catch(const std::exception &e) {
            std::cerr << "Mesh load failed. Reason: " << e.what() << "\nAborting.\n";
            return EXIT_FAILURE;
        }
    } else {
        std::cerr << utility::sprintfpp("Usage: %s mesh=[obj file]\n", argv[0]);
        return EXIT_SUCCESS;
    }

    //Step 3: Initialize objects
    init();

    //Step 4: Main loop
    while(!glfwWindowShouldClose(context->window)) {
        render_forward();
        update_title();

        glfwSwapBuffers(context->window);
        glfwPollEvents();
    }

    return EXIT_SUCCESS;
}