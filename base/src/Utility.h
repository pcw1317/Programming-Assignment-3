#ifndef UTILITY_H_
#define UTILITY_H_

#include <GL/glew.h>
#include <cstdlib>
#include <cstdarg>
#include <memory>
#include <string>

namespace utility 
{

	typedef struct 
	{
		GLuint vertex;
		GLuint fragment;
	} shaders_t;

	shaders_t loadShaders(const char * vert_path, const char * frag_path);
	GLuint loadComputeShader(const char * compute_path);

	void attachAndLinkProgram( GLuint program, shaders_t shaders);
	void attachAndLinkCSProgram (GLuint program, GLuint cshader);

	char* loadFile(const char *fname, GLint &fSize);

	// printShaderInfoLog
	// From OpenGL Shading Language 3rd Edition, p215-216
	// Display (hopefully) useful error messages if shader fails to compile
	void printShaderInfoLog(GLint shader);
	void printLinkInfoLog(GLint prog);
	void printFramebufferStatus(GLenum framebufferStatus);
	void printGLError(char * printString);

	std::string sprintfpp(const char *fmt_str, ...); 
}
 
#endif
