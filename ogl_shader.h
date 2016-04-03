#pragma once

#include <string>
#include <GL/gl3w.h>

GLuint load_shaders(const std::string& vertex_shader_text, 
					const std::string& fragment_shader_text, 
					std::string& shader_errors);
