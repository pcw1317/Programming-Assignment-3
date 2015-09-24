#ifndef _MAIN_H_
#define _MAIN_H_

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstring>
#include <tiny_obj_loader/tiny_obj_loader.h>

#include "gl_snippets.h"

//GL shader/program definitions
GLS_PROGRAM_DEFINE(
    kProgramSceneDraw,
    "res/shaders/forward_vert.glsl",
    "res/shaders/forward_frag.glsl",
{ "Position", "Normal" },
{ "outColor" },
{ "u_ModelMat", "u_ViewMat" , "u_PerspMat", "u_vplPosition", "u_vplIntensity", "u_vplDirection", "u_numLights", "u_AmbientColor", "u_DiffuseColor" }
);

GLS_PROGRAM_DEFINE(
    kProgramQuadDraw,
    "res/shaders/post.vert",
    "res/shaders/post.frag",
{ "Position", "Texcoord" },
{ "outColor" },
{ "u_Tex" }
);

constexpr float PI = 3.14159f;
constexpr float kFarPlane = 2000.f;
constexpr float kNearPlane = 0.1f;

enum gls_program_t {
    kGlsProgramSceneDraw,
    kGlsProgramQuadDraw,
    kGlsProgramMax
};

enum gls_buffer_t {
    kGlsBufferQuadVertexBuffer,
    kGlsBufferQuadIndexBuffer,
    kGlsBufferMax
};

enum gls_vertex_array_t {
    kGlsVertexArrayQuad,
    kGlsVertexArrayMax
};

enum gls_framebuffer_t {
	kGlsFramebufferScreen,
    kGlsFramebufferSceneDraw,
    kGlsFramebufferAccumulate,
    kGlsFramebufferMax
};

enum gls_texture_t {
    kGlsTextureScene,
    kGlsTextureAccumulate,
    kGlsTextureMax
};

typedef struct {
    unsigned int vertex_array;
    unsigned int vbo_indices;
    unsigned int num_indices;
    //Don't need these to get it working, but needed for deallocation
    unsigned int vbo_data;
} device_mesh2_t;

typedef struct {
    glm::vec3 pt;
    glm::vec2 texcoord;
} vertex2_t;

namespace quad_attributes {

enum {
    POSITION,
    TEXCOORD
};

}

enum Display {
    DISPLAY_DEPTH = 0,
    DISPLAY_NORMAL = 1,
    DISPLAY_POSITION = 2,
    DISPLAY_COLOR = 3,
    DISPLAY_TOTAL = 4,
    DISPLAY_LIGHTS = 5,
    DISPLAY_GLOWMASK = 6,
    DISPLAY_SHADOW = 7,
    DISPLAY_LPOS = 8
};

enum Render {
    RENDER_CAMERA = 0,
    RENDER_LIGHT = 1
};

#endif
