#include "Utility.h"

#include <cmath>
#include <iostream>
#include <fstream>
#include <string>

namespace Utility 
{

	char* loadFile(const char *fname, GLint &fSize)
	{
		std::ifstream::pos_type size;
		char * memblock;
		std::string text;

		// file read based on example in cplusplus.com tutorial
		std::ifstream file (fname, std::ios::in|std::ios::binary|std::ios::ate);
		if (file.is_open())
		{
			size = file.tellg();
			fSize = (GLuint) size;
			memblock = new char [size];
			file.seekg (0, std::ios::beg);
			file.read (memblock, size);
			file.close();
			std::cout << "file " << fname << " loaded" << std::endl;
			text.assign(memblock);
		}
		else
		{
			std::cout << "Unable to open file " << fname << std::endl;
			std::cin.get ();
			exit(1);
		}
		return memblock;
	}

	// printShaderInfoLog
	// From OpenGL Shading Language 3rd Edition, p215-216
	// Display (hopefully) useful error messages if shader fails to compile
	void printShaderInfoLog(GLint shader)
	{
		int infoLogLen = 0;
		int charsWritten = 0;
		GLchar *infoLog;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

		// should additionally check for OpenGL errors here

		if (infoLogLen > 0)
		{
			infoLog = new GLchar[infoLogLen];
			// error check for fail to allocate memory omitted
			glGetShaderInfoLog(shader,infoLogLen, &charsWritten, infoLog);
			std::cout << "InfoLog:" << std::endl << infoLog << std::endl;
			delete [] infoLog;
		}

		// should additionally check for OpenGL errors here
	}

	void printLinkInfoLog(GLint prog) 
	{
		int infoLogLen = 0;
		int charsWritten = 0;
		GLchar *infoLog;

		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLen);

		// should additionally check for OpenGL errors here

		if (infoLogLen > 0)
		{
			infoLog = new GLchar[infoLogLen];
			// error check for fail to allocate memory omitted
			glGetProgramInfoLog(prog,infoLogLen, &charsWritten, infoLog);
			std::cout << "InfoLog:" << std::endl << infoLog << std::endl;
			delete [] infoLog;
		}
	}

	shaders_t loadShaders(const char * vert_path, const char * frag_path) 
	{
		GLuint f, v;

		char *vs,*fs;

		v = glCreateShader(GL_VERTEX_SHADER);
		f = glCreateShader(GL_FRAGMENT_SHADER);	

		// load shaders & get length of each
		GLint vlen;
		GLint flen;
		vs = loadFile(vert_path,vlen);
		fs = loadFile(frag_path,flen);

		const char * vv = vs;
		const char * ff = fs;

		glShaderSource(v, 1, &vv,&vlen);
		glShaderSource(f, 1, &ff,&flen);

		GLint compiled;

		glCompileShader(v);
		glGetShaderiv(v, GL_COMPILE_STATUS, &compiled);
		if (!compiled)
		{
			std::cout << "Vertex shader not compiled." << std::endl;
			printShaderInfoLog(v);
		} 

		glCompileShader(f);
		glGetShaderiv(f, GL_COMPILE_STATUS, &compiled);
		if (!compiled)
		{
			std::cout << "Fragment shader not compiled." << std::endl;
			printShaderInfoLog(f);
		} 
		shaders_t out; out.vertex = v; out.fragment = f;

		delete [] vs; // dont forget to free allocated memory
		delete [] fs; // we allocated this in the loadFile function...

		return out;
	}

	GLuint loadComputeShader(const char * compute_path) 
	{
		GLuint c;

		char *cs;

		c = glCreateShader(GL_COMPUTE_SHADER);

		// load shader & get its length
		GLint clen;
		cs = loadFile(compute_path,clen);

		const char * cc = cs;

		glShaderSource(c, 1, &cc,&clen);

		GLint compiled;

		glCompileShader(c);
		glGetShaderiv(c, GL_COMPILE_STATUS, &compiled);
		if (!compiled)
		{
			std::cout << "Compute shader not compiled." << std::endl;
			printShaderInfoLog(c);
		} 

		delete [] cs;	// dont forget to free allocated memory
						// we allocated this in the loadFile function...
		return c;
	}

	void attachAndLinkProgram( GLuint program, shaders_t shaders) 
	{
		glAttachShader(program, shaders.vertex);
		glAttachShader(program, shaders.fragment);

		glLinkProgram(program);
		GLint linked;
		glGetProgramiv(program,GL_LINK_STATUS, &linked);
		if (!linked) 
		{
			std::cout << "Program did not link." << std::endl;
			printLinkInfoLog(program);
		}
	}

	void attachAndLinkCSProgram (GLuint program, GLuint computeshader) 
	{
		glAttachShader (program, computeshader);
		glLinkProgram(program);

		GLint linked;
		glGetProgramiv(program,GL_LINK_STATUS, &linked);
		if (!linked) 
		{
			std::cout << "Program did not link." << std::endl;
			printLinkInfoLog(program);
		}
	}

	void printFramebufferStatus(GLenum framebufferStatus)
	{
		switch (framebufferStatus)
		{
		case GL_FRAMEBUFFER_COMPLETE_EXT: break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			printf("Attachment Point Unconnected\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			printf("Missing Attachment\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			printf("Dimensions do not match\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			printf("Formats\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			printf("Draw Buffer\n");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			printf("Read Buffer\n");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			printf("Unsupported Framebuffer Configuration\n");
			break;
		default:
			printf("Unkown Framebuffer Object Failure\n");
			break;
		}
	}

	void printGLError(char * printString)
	{
		GLenum glError = glGetError();
		switch (glError)
		{
		case GL_INVALID_VALUE:
			std::cout << "\nInvalid value" << printString;
			std::cin.get();
			exit(1);
		case GL_INVALID_OPERATION:
			std::cout << "\nInvalid operation" << printString;
			std::cin.get();
			exit(1);
		case GL_INVALID_ENUM:
			std::cout << "\nInvalid enum" << printString;
			std::cin.get();
			exit(1);
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			std::cout << "\nThe framebuffer object is not complete" << printString;
			std::cin.get();
			exit(1);
		case GL_OUT_OF_MEMORY:
			std::cout << "\nThere is not enough memory left to execute the command" << printString;
			std::cin.get();
			exit(1);
		case GL_STACK_UNDERFLOW:
			std::cout << "\nAn attempt has been made to perform an operation that would cause an internal stack to underflow" << printString;
			std::cin.get();
			exit(1);
		case GL_STACK_OVERFLOW:
			std::cout << "\nAn attempt has been made to perform an operation that would cause an internal stack to overflow" << printString;
			std::cin.get();
			exit(1);
		case GL_NO_ERROR:
			break;
		default:
			std::cout << "\nError No.: " << glError << printString;
			std::cin.get();
			exit(1);
		}
	}
}
