#ifndef _MAIN_H_
#define _MAIN_H_

#include <vector>
#include <cstring>

#include <GL/glew.h>
#include <glm/glm.hpp>
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
{ "Position",  "Normal", "Texcoord" },
{ "outColor" },
{ "u_Tex" }
);

constexpr float PI = 3.14159f;
constexpr float kFarPlane = 2000.f;
constexpr float kNearPlane = 0.1f;

enum gls_program_t
{
    kGlsProgramSceneDraw,
    kGlsProgramQuadDraw,
    kGlsProgramMax
};

enum gls_buffer_t
{
    kGlsBufferQuadVertexBuffer,
    kGlsBufferQuadIndexBuffer,
    kGlsBufferMax
};

enum gls_vertex_array_t
{
    kGlsVertexArrayQuad,
    kGlsVertexArrayMax
};

enum gls_framebuffer_t
{
    kGlsFramebufferScreen,
    kGlsFramebufferSceneDraw,
    kGlsFramebufferAccumulate,
    kGlsFramebufferMax
};

enum gls_texture_t
{
    kGlsTextureScene,
    kGlsTextureAccumulate,
    kGlsTextureMax
};

enum display_type_t
{
    kDisplayTypeDepth = 0,
    kDisplayTypeNormal = 1,
    kDisplayTypePosition = 2,
    kDisplayTypeColor = 3,
    kDisplayTypeTotal = 4,
    kDisplayTypeLights = 5,
    kDisplayTypeGlowMask = 6,
    kDisplayTypeShadow = 7
};

#endif
