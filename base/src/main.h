#ifndef MAIN_H
#define MAIN_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstring>

#include "tiny_obj_loader.h"

enum Textures
{
	TEX_SCENE,
	TEX_ACCUM,
	TEX_MAX
};

enum FBOs
{
	FBO_SCENEDRAW,
	FBO_ACCUMULATE,
	FBO_MAX
};

enum Progs
{
	PROG_SCENEDRAW,
	PROG_QUADDRAW,
	PROG_MAX
};

const GLenum internalFormats[TEX_MAX] =
{ GL_RGBA32F, GL_RGBA32F };
const GLenum formats[TEX_MAX] =
{ GL_RGBA, GL_RGBA };
const GLenum types[TEX_MAX] =
{ GL_FLOAT, GL_FLOAT };

std::vector<tinyobj::shape_t> shapes;
GLuint textures[TEX_MAX];
GLuint fbo[FBO_MAX];
GLuint progs[PROG_MAX];

typedef struct 
{
	unsigned int vertex_array;
	unsigned int vbo_indices;
	unsigned int num_indices;
	//Don't need these to get it working, but needed for deallocation
	unsigned int vbo_data;
} device_mesh2_t;

typedef struct 
{
	glm::vec3 pt;
	glm::vec2 texcoord;
} vertex2_t;

namespace quad_attributes 
{
    enum 
	{
        POSITION,
        TEXCOORD
    };
}
enum Display 
{
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

enum Render
{
    RENDER_CAMERA = 0,
    RENDER_LIGHT = 1
};

char* loadFile(char *fname, GLint &fSize);
void printShaderInfoLog(GLint shader);
void printLinkInfoLog(GLint prog);
void initShade();
void initPass();

void initMesh();

void display(void);
void keyboard(unsigned char, int, int);
void reshape(int, int);

//int main (int argc, char* argv[]);

void    initNoise();
void    shaderInit();
void    initFBO(int width, int height);
void    init();
void    initMesh();
void    initQuad();
void	initVPL ();

void	checkError (char * printString);

#endif
