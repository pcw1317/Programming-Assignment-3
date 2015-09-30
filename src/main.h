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
    "res/shaders/screen_draw.vertex.glsl",
    "res/shaders/screen_draw.fragment.glsl",
{ "Position", "Normal" },
{ "outColor" },
{ "u_modelMat", "u_viewMat" , "u_perspMat", "u_vplPosition", "u_vplIntensity", "u_vplDirection", "u_numLights", "u_ambientColor", "u_diffuseColor", "u_shadowTex" }
);

GLS_PROGRAM_DEFINE(
    kProgramQuadDraw,
    "res/shaders/quad.vertex.glsl",
    "res/shaders/quad.fragment.glsl",
{ "Position",  "Normal", "Texcoord" },
{ "outColor" },
{ "u_Tex" }
);

GLS_PROGRAM_DEFINE(
    kProgramShadowMapping,
    "res/shaders/shadow.vertex.glsl",
    "res/shaders/shadow.fragment.glsl",
{ "Position" },
{ "outColor" },
{ "u_model", "u_cameraToShadowView" , "u_cameraToShadowProjector" }
);

constexpr float PI = 3.14159f;
constexpr float kFarPlane = 2000.f;
constexpr float kNearPlane = 0.1f;
constexpr int kShadowSize = 256;
constexpr int kVplCount = 128;

enum gls_program_t
{
    kGlsProgramSceneDraw,
    kGlsProgramQuadDraw,
    kGlsProgramShadowMapping,
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

enum gls_cubemap_framebuffer_t
{
    kGlsCubemapFramebufferShadow,
    kGlsCubemapFramebufferMax
};

enum gls_texture_t
{
    kGlsTextureScene,
    kGlsTextureAccumulate,
    kGlsTextureMax
};

#endif
