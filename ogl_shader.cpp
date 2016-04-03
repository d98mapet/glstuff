#include "ogl_shader.h"

namespace {
bool compile_shader(GLuint shader_id, const std::string &shader_text, std::string &compile_error) {
	const char *shader_text_p = shader_text.c_str();
	glShaderSource(shader_id, 1, &shader_text_p, NULL);
	glCompileShader(shader_id);

	GLint result = GL_FALSE;
	int info_log_length;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &info_log_length);
	if (info_log_length > 0){
		compile_error.resize(info_log_length + 1);
		glGetShaderInfoLog(shader_id, info_log_length, NULL, &compile_error[0]);
		return false;
	}
	return true;
}
}

GLuint load_shaders(const std::string& vertex_shader_text, 
					const std::string& fragment_shader_text, 
					std::string& shader_errors) {
	GLuint vs_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint fs_id = glCreateShader(GL_FRAGMENT_SHADER);

	std::string error;
	if (!compile_shader(vs_id, vertex_shader_text, error)) {
		shader_errors.append(error);
	}
	if (!compile_shader(fs_id, fragment_shader_text, error)) {
		shader_errors.append(error);
	}

	GLuint program_id = glCreateProgram();
	glAttachShader(program_id, vs_id);
	glAttachShader(program_id, fs_id);
	glLinkProgram(program_id);
	glDetachShader(program_id, vs_id);
	glDetachShader(program_id, fs_id);
	glDeleteShader(vs_id);
	glDeleteShader(fs_id);
	return program_id;
}
