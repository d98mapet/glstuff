// ImGui - standalone example application for Glfw + OpenGL 3, using programmable pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "parser.h"
#include "ogl_shader.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <regex>
#include <chrono>

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error %d: %s\n", error, description);
}

enum uniformtype {
	UT_FLOAT,
	UT_MAT4,
	UT_SAMP2D,
	UT_BUFFER
};

struct UniformData{
	uniformtype type;
	std::string name;
	unsigned int size;
	void* uniform_data;
};

struct TextureBinding {
	std::string uniform_name;
	GLuint texture;
	GLenum target;
};

void create_uniform_float(std::string name, float value, float minValue, float maxValue, UniformData& uniform) {
	uniform.name = name;
	uniform.type = uniformtype::UT_FLOAT;
	uniform.size = 3;
	uniform.uniform_data = malloc(sizeof(float) * uniform.size);
	float data[3]; // make into vector?
	data[0] = value;
	data[1] = minValue;
	data[2] = maxValue;
	memcpy(uniform.uniform_data, &data[0], sizeof(float)*uniform.size);
}

void create_uniform_sampler2d(std::string name, std::string file, UniformData& uniform) {
	uniform.name = name;
	uniform.type = uniformtype::UT_SAMP2D;
	uniform.size = file.size() + 1;
	uniform.uniform_data = malloc(uniform.size);
	memcpy(uniform.uniform_data, file.c_str(), sizeof(char)*uniform.size);
}

void create_uniform_buffer_sampler(const std::string &name, UniformData &uniform) {
	uniform.name = name;
	uniform.type = uniformtype::UT_BUFFER;
	uniform.size = 0;
	uniform.uniform_data = 0;
}

void delete_uniform_data(UniformData &uniform) {
	free(uniform.uniform_data);
}

bool parse_uniform(const std::string &line, UniformData &ud) {
	UniformParseResult result;
	if (parse_uniform(line, result)) {
		if (result.uniform_type == "float") {
			if (result.uniform_name == "iGlobalTime") {
				return false;
			}
			float value = 0.5f;
			float minValue = 0.0f;
			float maxValue = 1.0f;
			parse_value(result.default_value, value);
			if (result.comment_name == "ui") {
				std::smatch m;
				const std::string re = "\\s*([-\\d\\.]+)\\s*,\\s*([-\\d\\.]+)";
				std::regex_search(result.comment_value, m, std::regex(re));
				if (!m.empty() && m.size() == 3) {
					parse_value(m[1], minValue);
					parse_value(m[2], maxValue);
				}
			}
			create_uniform_float(result.uniform_name, value, minValue, maxValue, ud);
			return true;
		} else if (result.uniform_type == "sampler2D") {
			create_uniform_sampler2d(result.uniform_name, result.comment_name == "path" ? result.comment_value : "", ud);
			return true;
		} else if (result.uniform_type == "usamplerBuffer") {
			create_uniform_buffer_sampler(result.uniform_name, ud);
			return true;
		}
	}
	return false;
}

void load_textures(const std::vector<UniformData>& ud, std::vector<TextureBinding>& texture_bindings) {
	texture_bindings.clear();
	for (unsigned int i = 0; i < ud.size(); i++) {
		if (ud[i].type == uniformtype::UT_SAMP2D) {
			if (ud[i].name == "input_map") {
				continue; //special case for off screen fb
			}
			TextureBinding tb;

			tb.target = GL_TEXTURE_2D;
			glGenTextures(1, &tb.texture);
			glBindTexture(GL_TEXTURE_2D, tb.texture);

			int x, y, n;
			char* filename = (char*)ud[i].uniform_data;
			unsigned char *data = stbi_load(filename, &x, &y, &n, 3);
			if (data == 0) {
				printf("Could not load texture %s\n", filename);
			}
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			tb.uniform_name = ud[i].name;
			texture_bindings.push_back(tb);

		} else if (ud[i].type == uniformtype::UT_BUFFER) {
			std::cout << "Creating buffer uniform " << ud[i].name << std::endl;
			TextureBinding tb;

			tb.target = GL_TEXTURE_BUFFER;
			GLuint buffer;
			glGenBuffers(1, &buffer);
			glBindBuffer(GL_TEXTURE_BUFFER, buffer);
float bvh[120] = {
-1.5, -1.5, -1.5, 1,
1.5, 1.5, 1.5, 0,
-1.5, -1.5, -1.5, 1,
1.5, 1.5, -0.5, 7,
-1.5, -1.5, -1.5, 1,
1.5, -0.5, -0.5, 3,
-1, -1, -1, -1,
0.5, 0, 0, 0,
1, -1, -1, -1,
0.5, 0, 0, 0,
-1.5, 0.5, -1.5, 1,
1.5, 1.5, -0.5, 3,
-1, 1, -1, -1,
0.5, 0, 0, 0,
1, 1, -1, -1,
0.5, 0, 0, 0,
-1.5, -1.5, 0.5, 1,
1.5, 1.5, 1.5, 0,
-1.5, -1.5, 0.5, 1,
1.5, -0.5, 1.5, 3,
-1, -1, 1, -1,
0.5, 0, 0, 0,
1, -1, 1, -1,
0.5, 0, 0, 0,
-1.5, 0.5, 0.5, 1,
1.5, 1.5, 1.5, 0,
-1, 1, 1, -1,
0.5, 0, 0, 0,
1, 1, 1, -1,
0.5, 0, 0, 0
};
			glBufferData(GL_TEXTURE_BUFFER, 120 * 4, bvh, GL_STATIC_DRAW);

			glGenTextures(1, &tb.texture);
			glBindTexture(GL_TEXTURE_BUFFER, tb.texture);
			glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32UI, buffer);
			tb.uniform_name = ud[i].name;
			texture_bindings.push_back(tb);
		}
	}
}

std::string read_file(std::istream &input_stream) {
	input_stream.seekg(0, std::ios::end);
	size_t size = input_stream.tellg();
	std::string buffer;
	buffer.resize(size);
	input_stream.seekg(0);
	input_stream.read(&buffer[0], size);
	return buffer;
}

GLuint load_shaders(const std::string& vs, const std::string& fs, std::vector<UniformData>& uniforms, std::string& shader_errors) {
	for (unsigned int i = 0; i < uniforms.size(); i++) {
		delete_uniform_data(uniforms[i]);
	}
	uniforms.clear();

	std::string vertex_code;
	std::string fragment_code;

	std::ifstream vtext(vs);
	if (vtext.is_open()) {
		vertex_code = read_file(vtext);
	}
	std::ifstream ftext(fs);
	if (ftext.is_open()) {
		fragment_code = read_file(ftext);
		std::string line;
		std::istringstream iss(fragment_code);
		UniformData ud;
		while (std::getline(iss, line)) {
			if (parse_uniform(line, ud)) {
				uniforms.push_back(ud);
			}
		}
	}
	GLuint program_id = load_shaders(vertex_code, fragment_code, shader_errors);
	if (shader_errors.length() > 0) {
		std::cout << shader_errors;
	}
	return program_id;

}

GLuint setup_buffer(GLfloat *data, unsigned int data_size) {
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, data_size, data, GL_STATIC_DRAW);
	return buffer;
}

void draw_stuff(GLuint vertexbuffer, GLuint uvbuffer) {

	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

}

void setup_render_texture(unsigned int width, unsigned int height, GLuint fb, GLuint rtt) {
	glBindFramebuffer(GL_FRAMEBUFFER, fb);
	glBindTexture(GL_TEXTURE_2D, rtt);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rtt, 0);
	GLenum db[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, db);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		return;
	}
}

int main(int, char**)
{
	const char *fragment_shader = "../../fragment3.glsl";

    // Setup window
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
//    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui OpenGL3 example", NULL, NULL);
    GLFWwindow* window = glfwCreateWindow(640, 480, "ImGui OpenGL3 example", NULL, NULL);

	glfwMakeContextCurrent(window);

	gl3wInit();

#if 1
	GLuint vaID;
	glGenVertexArrays(1, &vaID);
	glBindVertexArray(vaID);
#endif

	
	// Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, true);

    // Load Fonts
    // (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
    //ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

    ImVec4 clear_color = ImColor(114, 144, 154);
#if 1
	std::vector<UniformData> uniforms;
	std::string shader_errors;
	GLuint pID = load_shaders("../../vertex.glsl", fragment_shader, uniforms, shader_errors);

	std::vector<TextureBinding> texture_bindings;
	load_textures(uniforms, texture_bindings);

//	float val = *(float*)(uniforms[0].uniform_data);
	GLfloat vertex_data[] = {
		-1.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		1.0f, 1.0f, 0.0f

 	};

	GLfloat uv_data[] = {
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};

	GLuint vertexbuffer;
	vertexbuffer = setup_buffer(&vertex_data[0], sizeof(vertex_data));
	GLuint uvbuffer;
	uvbuffer = setup_buffer(&uv_data[0], sizeof(uv_data));

#endif
	int display_w = 0, display_h = 0;
	GLuint fb;
	GLuint rtt;
	glGenFramebuffers(1, &fb);
	glGenTextures(1, &rtt);

	std::vector<UniformData> uniforms_fb;
	std::string shader_errors_fb;

	GLuint fbpid = load_shaders("../../vertex.glsl", "../../fragment.glsl", uniforms_fb, shader_errors_fb);
	GLuint fbTexID = glGetUniformLocation(fbpid, "input_map");
	// Main loop

	GLfloat time = 0.0f;

	float render_resolution_scale = 1.f / 3.f;
	bool restart = false;
    while (!glfwWindowShouldClose(window))
    {  
		glBindFramebuffer(GL_FRAMEBUFFER, fb);

		glfwPollEvents();

        ImGui_ImplGlfwGL3_NewFrame();

        // 1. Show a simple window
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
        {
            //static float f = 0.0f;
            //ImGui::Text("Hello, world!");
            //ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            //ImGui::ColorEdit3("clear color", (float*)&clear_color);
            //if (ImGui::Button("Test Window")) show_test_window ^= 1;
            //if (ImGui::Button("Another Window")) show_another_window ^= 1;
			if (ImGui::Button("reload shader")) {
				shader_errors.clear();
				pID = load_shaders("../../vertex.glsl", fragment_shader, uniforms, shader_errors);
				load_textures(uniforms, texture_bindings);
				restart = true;
			}
			{
				float x = render_resolution_scale;
				ImGui::SliderFloat("Render Scale", &x, 0.1f, 2.f);
				if (x != render_resolution_scale) {
					render_resolution_scale = x;
					restart = true;
				}
			}
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			if (shader_errors.size() > 0) {
				ImGui::Text("%s", &shader_errors[0]);
			}

		}

        // 2. Show another simple window, this time using an explicit Begin/End pair
		if (uniforms.size() != 0) {
            ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Shader Parameters");
//            ImGui::Text("Hello");
			for (unsigned int i = 0; i < uniforms.size(); i++) {
				if (uniforms[i].type != uniformtype::UT_FLOAT) {
					continue;
				}
				float* float_data = (float*)((uniforms[i].uniform_data));
				float temp = *float_data;
				//float test = *stuff;
				//float test1 = *(stuff+1);
				//float test2 = *(stuff+2);
				if (uniforms[i].type == uniformtype::UT_FLOAT) {
					ImGui::SliderFloat(uniforms[i].name.c_str(), 
										float_data, 
										*(float_data+1),
										*(float_data+2));
				}
				if (temp != *float_data) {
					restart = true;
				}
			}
            ImGui::End();
        }


		// Rendering
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			if (width != display_w || height != display_h) {
				display_w = width;
				display_h = height;
				restart = true;
			}
		}

		glViewport(0, 0, display_w * render_resolution_scale, display_h * render_resolution_scale);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		if (restart) {
			setup_render_texture(display_w * render_resolution_scale, display_h * render_resolution_scale, fb, rtt);
			glClear(GL_COLOR_BUFFER_BIT);
			time = 0;
			restart = false;
		}
		GLint uniformID = glGetUniformLocation(pID, "iGlobalTime");
		glProgramUniform1f(pID, uniformID, time);
		uniformID = glGetUniformLocation(fbpid, "iGlobalTime");
		glProgramUniform1f(fbpid, uniformID, time);
		time = time + 1.0f;
		
		//uniformID = glGetUniformLocation(pID, "iResolution");
		//glProgramUniform2f(pID, uniformID, (float)display_w, float(display_h));

		for (unsigned int i = 0; i < uniforms.size(); i++) {
			if (uniforms[i].type == uniformtype::UT_FLOAT) {
				//if (uniforms[i].name == "iGlobalTime"){ //Move this out of main loop, maybe cull list?
				//	continue;
				//}
				uniformID = glGetUniformLocation(pID, uniforms[i].name.c_str());
				glProgramUniform1f(pID, uniformID, *(float *)(uniforms[i].uniform_data));
			}
		}

		glUseProgram(pID);

		for (unsigned int i = 0; i < texture_bindings.size(); i++) {
			uniformID = glGetUniformLocation(pID, texture_bindings[i].uniform_name.c_str());
			glUniform1i(uniformID, i);
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(texture_bindings[i].target, texture_bindings[i].texture);
		}

		//special off screen render texture
		if (texture_bindings.size() > 0) {
			uniformID = glGetUniformLocation(pID, "input_map");
			GLuint unit = texture_bindings.size();
			glUniform1i(uniformID, unit);
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(GL_TEXTURE_2D, rtt);
		}
		draw_stuff(vertexbuffer, uvbuffer);

		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, display_w, display_h);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(fbpid);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, rtt);
		glProgramUniform1i(fbpid, fbTexID, 0);
		draw_stuff(vertexbuffer, uvbuffer);
		ImGui::Render();

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();

    return 0;
}
