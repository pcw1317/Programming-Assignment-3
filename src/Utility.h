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

	namespace _scope_exit_detail {
		//C++11-style ScopeExit
		template <typename ExitFuncType>
		struct scope_guard {
			scope_guard(ExitFuncType f) : f(f), run(true) {}
			scope_guard(const scope_guard& rhs) = delete;
			scope_guard(scope_guard&& rhs): f(rhs.f), run(true)	{ rhs.run = false; }
			~scope_guard() { if(run) f(); }
			scope_guard& operator=(const scope_guard& rhs) = delete;
			scope_guard& operator=(scope_guard&& rhs) {
				f = rhs.f;
				rhs.run = false;
				run = true;
			}
			ExitFuncType f;
			bool run;
		};
#		define SCOPE_EXIT_STRJOIN(a, b) SCOPE_EXIT_STRJOIN_INDIRECT(a, b)
#		define SCOPE_EXIT_STRJOIN_INDIRECT(a, b) a ## b
	}

	template <typename ExitFuncType>
	_scope_exit_detail::scope_guard<ExitFuncType> scope_exit(ExitFuncType f) {
		return _scope_exit_detail::scope_guard<ExitFuncType>(f);
	};
#define SCOPE_EXIT(f) auto SCOPE_EXIT_STRJOIN(_scope_exit_, __LINE__) = utility::scope_exit(f)


}
 
#endif
