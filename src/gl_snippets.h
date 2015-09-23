#ifndef _GL_SNIPPETS_H_
#define _GL_SNIPPETS_H_

#include <string>
#include <GL/glew.h>
#include <utility.h>
#include <deque>

namespace gls {
	class shader {
	public:
		shader(const char *path, GLenum type): path_(path), type_(type) {
			std::string shader_source = utility::read_file(path);

			shader_ = glCreateShader(type);
			{
				const char *shader_source_ptr = shader_source.c_str();
				GLint shader_source_length = GLint(shader_source.size());
				glShaderSource(shader_, 1, &shader_source_ptr, &shader_source_length);
			}
			glCompileShader(shader_);

			//check failure
			{
				GLint compiled;
				glGetShaderiv(shader_, GL_COMPILE_STATUS, &compiled);

				if (!compiled) {
					//fetch error log
					std::string infolog;
					{
						GLint info_log_len;
						glGetShaderiv(shader_, GL_INFO_LOG_LENGTH, &info_log_len);
						if (info_log_len > 0) {
							infolog.resize(info_log_len);
							glGetShaderInfoLog(shader_, info_log_len, nullptr, &infolog[0]);
						}
					}
					
					throw std::runtime_error(
						utility::sprintfpp(
							"shader (%s) compilation failure; reason: %s",
							path,
							infolog.c_str()
							).c_str());
				}
			}
		}
		shader(const shader&) = delete;
		shader& operator=(const shader&) = delete;
		~shader() {
			if(shader_)
				glDeleteShader(shader_);
		}
		shader(shader&& rhs) {
			*this = std::move(rhs);
		}
		shader& operator=(shader&& rhs) {
			shader_ = rhs.shader_;
			rhs.shader_ = 0;
			return *this;
		}
		GLuint get() const { return shader_; }
	private:
		GLuint shader_;
		GLenum type_;
		std::string path_;
	};

	struct _program_definition {
		const char *vertex_shader_path;
		const char *fragment_shader_path;
		std::initializer_list<const char*> attribute_names;
		std::initializer_list<const char*> frag_data_names;
		std::initializer_list<const char*> uniform_names;
	};

#define GLS_PROGRAM_DEFINE(name, ...) \
	const ::gls::_program_definition name = {__VA_ARGS__}

	class program {
	public:
		program(const _program_definition &pdef) : program(pdef.vertex_shader_path, pdef.fragment_shader_path, pdef.attribute_names, pdef.frag_data_names, pdef.uniform_names){}

		template<class AttributeNameContainerType, class FragDataNameContainerType, class UniformNameContainerType>
		program(
			const char *vertex_shader_path,
			const char *fragment_shader_path,
			AttributeNameContainerType attribute_names,
			FragDataNameContainerType frag_data_names,
			UniformNameContainerType uniform_names
			) {
			program_ = glCreateProgram();
			shaders_.push_back(shader(vertex_shader_path, GL_VERTEX_SHADER));
			shaders_.push_back(shader(fragment_shader_path, GL_FRAGMENT_SHADER));
			for (auto it = std::begin(shaders_); it != std::end(shaders_); ++it)
				glAttachShader(program_, it->get());
			glLinkProgram(program_);
			
			//check failure
			{
				GLint linked;
				glGetProgramiv(program_, GL_LINK_STATUS, &linked);

				if (!linked) {
					//fetch error log
					std::string infolog;
					{
						GLint info_log_len;
						glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &info_log_len);
						if (info_log_len > 0) {
							infolog.resize(info_log_len);
							glGetProgramInfoLog(program_, info_log_len, nullptr, &infolog[0]);
						}
					}

					throw std::runtime_error(
						utility::sprintfpp(
							"program link failure; reason: %s",
							infolog.c_str()
							).c_str());
				}
			}

			set_attribute_map(attribute_names);
			set_frag_data_map(frag_data_names);
			set_uniform_map(uniform_names);
		}

		template<class AttributeNameContainerType>
		void set_attribute_map(AttributeNameContainerType attribute_names) {
			GLuint i = 0;
			for (auto it = std::begin(attribute_names); it != std::end(attribute_names); ++it)
				glBindAttribLocation(program_, i++, *it);
		}

		template<class FragDataNameContainerType>
		void set_frag_data_map(FragDataNameContainerType frag_data_names) {
			GLuint i = 0;
			for (auto it = std::begin(frag_data_names); it != std::end(frag_data_names); ++it)
				glBindFragDataLocation(program_, i++, *it);
		}

		template<class UniformNameContainerType>
		void set_uniform_map(UniformNameContainerType uniform_names) {
			uniform_locations_.clear();
			for (auto it = std::begin(uniform_names); it != std::end(uniform_names); ++it)
				uniform_locations_.push_back(glGetUniformLocation(program_, *it));
		}
		template<class ValueType>
		void set_uniform(GLuint index, ValueType value);

		program(const program&) = delete;
		program& operator=(const program&) = delete;
		~program() {
			if (program_)
				glDeleteProgram(program_);
		}
		program(program&& rhs) {
			*this = std::move(rhs);
		}
		program& operator=(program&& rhs) {
			program_ = rhs.program_;
			rhs.program_ = 0;
			shaders_ = std::move(rhs.shaders_);
			uniform_locations_ = std::move(rhs.uniform_locations_);
			return *this;
		}
	private:
		std::vector<shader> shaders_;
		GLuint program_;
		std::vector<GLuint> uniform_locations_;
	};


	//glUniform1d;
	//glUniform1f;
	//glUniform1i;
	//glUniform1ui;

	//glUniform2d;
	//glUniform2f;
	//glUniform2i;
	//glUniform2ui;

	//glUniform3d;
	//glUniform3f;
	//glUniform3i;
	//glUniform3ui;

	//glUniform4d;
	//glUniform4f;
	//glUniform4i;
	//glUniform2ui;

	//glUniformMatrix2dv;
	//glUniformMatrix2fv;
	//glUniformMatrix2x3dv;
	//glUniformMatrix2x3fv;
	//glUniformMatrix2x4dv;
	//glUniformMatrix2x4fv;
	//
	//glUniformMatrix3dv;
	//glUniformMatrix3fv;
	//glUniformMatrix3x2dv;
	//glUniformMatrix3x2fv;
	//glUniformMatrix3x4dv;
	//glUniformMatrix3x4fv;

	//glUniformMatrix4dv;
	//glUniformMatrix4fv;
	//glUniformMatrix4x2dv;
	//glUniformMatrix4x2fv;
	//glUniformMatrix4x3dv;
	//glUniformMatrix4x3fv;

	template<> inline void program::set_uniform<double>(GLuint index, double value) { glUniform1d(uniform_locations_[index], value); }
	template<> inline void program::set_uniform<float>(GLuint index, float value) { glUniform1f(uniform_locations_[index], value); }
	template<> inline void program::set_uniform<int>(GLuint index, int value) { glUniform1i(uniform_locations_[index], value); }
	template<> inline void program::set_uniform<unsigned int>(GLuint index, unsigned int value) { glUniform1ui(uniform_locations_[index], value); }
}

#endif