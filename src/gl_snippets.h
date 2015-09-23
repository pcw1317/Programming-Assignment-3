//Configuration macros:
//	GLS_NO_BIND_CHECK: no bind checking in debug mode
//

#ifndef _GL_SNIPPETS_H_
#define _GL_SNIPPETS_H_

#include <string>
#include <GL/glew.h>
#include <utility.h>
#include <glm/gtc/type_ptr.hpp>

namespace gls {
	namespace _gls_detail {
		template<typename T>
		inline void check_bound(const T &obj) {
#			if defined(_DEBUG) && !defined(GLS_NO_BIND_CHECK)
			GLint ret;
			glGetIntegerv(T::kBindParameterName, &ret);
			assert(obj.get() == ret);
#			endif
		}
	}
	class shader {
	public:
		//common interfaces
		GLuint get() const { return shader_; }

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
		shader(shader&& rhs): shader_(0) {
			*this = std::move(rhs);
		}
		shader& operator=(shader&& rhs) {
			if(shader_)
				glDeleteShader(shader_);

			shader_ = rhs.shader_;
			rhs.shader_ = 0;
			type_ = rhs.type_;
			path_ = std::move(rhs.path_);
			return *this;
		}
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
		//common interfaces
		static constexpr GLenum kBindParameterName = GL_CURRENT_PROGRAM;
		GLuint get() const { return program_; }
		void bind() { glUseProgram(program_); }
		void unbind() { glUseProgram(0); }

	public:
		program() : program_(0) {}
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

			set_attribute_map(attribute_names);
			set_frag_data_map(frag_data_names);


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
		void set_uniform(GLuint index, const ValueType &value);

		template<typename... ArgsType>
		void set_uniforms(ArgsType&&... args) { set_uniforms_from(0, std::forward<ArgsType>(args)...); }

		template<typename... ArgsType>
		void set_uniforms_from(GLuint index_from, ArgsType&&... args) {
			set_uniforms_from_impl_<ArgsType...>::run(*this, index_from, std::forward<ArgsType>(args)...);
		}

		program(const program&) = delete;
		program& operator=(const program&) = delete;
		~program() {
			if (program_)
				glDeleteProgram(program_);
		}
		program(program&& rhs): program_(0) {
			*this = std::move(rhs);
		}
		program& operator=(program&& rhs) {
			if (program_)
				glDeleteProgram(program_);

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

		template<typename... ArgsType>
		struct set_uniforms_from_impl_;

		template<typename FirstArgType, typename... ArgsType>
		struct set_uniforms_from_impl_<FirstArgType, ArgsType...> {
			static void run(program &p, GLuint index_from, FirstArgType&& first_arg, ArgsType&&... args) {
				p.set_uniform(index_from, std::forward<FirstArgType>(first_arg));
				set_uniforms_from_impl_<ArgsType...>::run(p, index_from + 1, std::forward<ArgsType>(args)...);
			}
		};

		template<>
		struct set_uniforms_from_impl_<> {
			static void run(program &p, GLuint index_from) {}
		};
	};

	template<> inline void program::set_uniform<double>(GLuint index, const double &value) { _gls_detail::check_bound(*this);  glUniform1d(uniform_locations_[index], value); }
	template<> inline void program::set_uniform<float>(GLuint index, const float &value) { _gls_detail::check_bound(*this);  glUniform1f(uniform_locations_[index], value); }
	template<> inline void program::set_uniform<int>(GLuint index, const int &value) { _gls_detail::check_bound(*this); glUniform1i(uniform_locations_[index], value); }
	template<> inline void program::set_uniform<unsigned int>(GLuint index, const unsigned int &value) { _gls_detail::check_bound(*this);  glUniform1ui(uniform_locations_[index], value); }

	template<> inline void program::set_uniform<glm::dvec2>(GLuint index, const glm::dvec2 &value) { _gls_detail::check_bound(*this);  glUniform2dv(uniform_locations_[index], 1, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::vec2>(GLuint index, const glm::vec2 &value) { _gls_detail::check_bound(*this);  glUniform2fv(uniform_locations_[index], 1, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::ivec2>(GLuint index, const glm::ivec2 &value) { _gls_detail::check_bound(*this);  glUniform2iv(uniform_locations_[index], 1, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::uvec2>(GLuint index, const glm::uvec2 &value) { _gls_detail::check_bound(*this);  glUniform2uiv(uniform_locations_[index], 1, glm::value_ptr(value)); }

	template<> inline void program::set_uniform<glm::dvec3>(GLuint index, const glm::dvec3 &value) { _gls_detail::check_bound(*this);  glUniform3dv(uniform_locations_[index], 1, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::vec3>(GLuint index, const glm::vec3 &value) { _gls_detail::check_bound(*this);  glUniform3fv(uniform_locations_[index], 1, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::ivec3>(GLuint index, const glm::ivec3 &value) { _gls_detail::check_bound(*this);  glUniform3iv(uniform_locations_[index], 1, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::uvec3>(GLuint index, const glm::uvec3 &value) { _gls_detail::check_bound(*this);  glUniform3uiv(uniform_locations_[index], 1, glm::value_ptr(value)); }

	template<> inline void program::set_uniform<glm::dvec4>(GLuint index, const glm::dvec4 &value) { _gls_detail::check_bound(*this);  glUniform4dv(uniform_locations_[index], 1, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::vec4>(GLuint index, const glm::vec4 &value) { _gls_detail::check_bound(*this);  glUniform4fv(uniform_locations_[index], 1, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::ivec4>(GLuint index, const glm::ivec4 &value) { _gls_detail::check_bound(*this);  glUniform4iv(uniform_locations_[index], 1, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::uvec4>(GLuint index, const glm::uvec4 &value) { _gls_detail::check_bound(*this);  glUniform4uiv(uniform_locations_[index], 1, glm::value_ptr(value)); }

	template<> inline void program::set_uniform<glm::dmat2>(GLuint index, const glm::dmat2 &value) { _gls_detail::check_bound(*this);  glUniformMatrix2dv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::mat2>(GLuint index, const glm::mat2 &value) { _gls_detail::check_bound(*this);  glUniformMatrix2fv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::dmat2x3>(GLuint index, const glm::dmat2x3 &value) { _gls_detail::check_bound(*this);  glUniformMatrix2x3dv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::mat2x3>(GLuint index, const glm::mat2x3 &value) { _gls_detail::check_bound(*this);  glUniformMatrix2x3fv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::dmat2x4>(GLuint index, const glm::dmat2x4 &value) { _gls_detail::check_bound(*this);  glUniformMatrix2x4dv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::mat2x4>(GLuint index, const glm::mat2x4 &value) { _gls_detail::check_bound(*this);  glUniformMatrix2x4fv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }

	template<> inline void program::set_uniform<glm::dmat3>(GLuint index, const glm::dmat3 &value) { _gls_detail::check_bound(*this);  glUniformMatrix3dv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::mat3>(GLuint index, const glm::mat3 &value) { _gls_detail::check_bound(*this);  glUniformMatrix3fv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::dmat3x2>(GLuint index, const glm::dmat3x2 &value) { _gls_detail::check_bound(*this);  glUniformMatrix3x2dv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::mat3x2>(GLuint index, const glm::mat3x2 &value) { _gls_detail::check_bound(*this);  glUniformMatrix3x2fv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::dmat3x4>(GLuint index, const glm::dmat3x4 &value) { _gls_detail::check_bound(*this);  glUniformMatrix3x4dv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::mat3x4>(GLuint index, const glm::mat3x4 &value) { _gls_detail::check_bound(*this);  glUniformMatrix3x4fv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }

	template<> inline void program::set_uniform<glm::dmat4>(GLuint index, const glm::dmat4 &value) { _gls_detail::check_bound(*this);  glUniformMatrix4dv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::mat4>(GLuint index, const glm::mat4 &value) { _gls_detail::check_bound(*this);  glUniformMatrix4fv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::dmat4x2>(GLuint index, const glm::dmat4x2 &value) { _gls_detail::check_bound(*this);  glUniformMatrix4x2dv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::mat4x2>(GLuint index, const glm::mat4x2 &value) { _gls_detail::check_bound(*this);  glUniformMatrix4x2fv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::dmat4x3>(GLuint index, const glm::dmat4x3 &value) { _gls_detail::check_bound(*this);  glUniformMatrix4x3dv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
	template<> inline void program::set_uniform<glm::mat4x3>(GLuint index, const glm::mat4x3 &value) { _gls_detail::check_bound(*this);  glUniformMatrix4x3fv(uniform_locations_[index], 1, GL_FALSE, glm::value_ptr(value)); }
}

#endif