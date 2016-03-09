// ImGui - standalone example application for Glfw + OpenGL 3, using programmable pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.

#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"
#include <stdio.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <string>
#include <fstream>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error %d: %s\n", error, description);
}

enum uniformtype {
	UT_FLOAT,
	UT_MAT4,
	UT_SAMP2D
};


struct UniformData{
	uniformtype type;
	std::string name;
	unsigned int size;
	void* uniform_data;

};

struct texture_binding {
	std::string sampler2D;
	GLuint texture;
	GLuint texture_unit;
};

void create_uniform_float(std::string name, float value, float minValue, float maxValue, UniformData& uniform) {
	uniform.name = name;
	uniform.type = uniformtype::UT_FLOAT;
	uniform.size = 3;
	uniform.uniform_data = new float[uniform.size];
	float data[3]; // make into vector?
	data[0] = value;
	data[1] = minValue;
	data[2] = maxValue;
	memcpy(uniform.uniform_data, &data[0], sizeof(float)*uniform.size);
		//uniform.uniform_data = &v;

}

void create_uniform_sampler2D(std::string name, std::string file, UniformData& uniform) {
	uniform.name = name;
	uniform.type = uniformtype::UT_SAMP2D;
	uniform.size = file.size() + 1;
	uniform.uniform_data = new char[uniform.size];
	memcpy(uniform.uniform_data, file.c_str(), sizeof(char)*uniform.size);

}

void delete_uniform_data(UniformData &uniform) {
	delete[] uniform.uniform_data;
}

void parseNameValueDefault(const std::string line, std::string &name, float &value, float &minValue, float &maxValue) {
	std::string valid_chars = "abcdefghijklmnopqrstuvwxyz_-";
	std::size_t start = line.find_first_of(valid_chars);
	std::size_t end = line.find_first_of(" =;", start);
	name = line.substr(start, end - start);
	std::string value_defaults = line.substr(end);
	//parse optional UI things
	start = value_defaults.find("ui(");
	if (start != std::string::npos) {
		start = start + 3;
		end = value_defaults.find_first_of(",", start);
		std::string tempS = value_defaults.substr(start, end - start);
		minValue = (float)atof(tempS.c_str());
		start = end+1;
		end = value_defaults.find_first_of(")", start);
		tempS = value_defaults.substr(start, end - start);
		maxValue = (float)atof(tempS.c_str());

	}


	start = value_defaults.find_first_of("=");
	if (start == std::string::npos) {
		return; // no default given
	}
	else {
		start += 1;
		end = value_defaults.find_first_of(";", start);
		std::string tempS = value_defaults.substr(start, end - start);
		value = (float)atof(tempS.c_str());
	}

}

bool parseUniformFloat(const std::string line, UniformData &ud) {
	std::size_t found = line.find("uniform");
	if (found == std::string::npos) {
		return false;
	}
	std::string temp_line = line.substr(found + 7); //strip uniform

	found = temp_line.find("float");
	if (found != std::string::npos) {
		std::string name;
		float value = 0.5f;
		float minValue = 0.0f;
		float maxValue = 1.0f;

		parseNameValueDefault(temp_line.substr(found+5), name, value, minValue, maxValue);
		if (name == "iGlobalTime") {
			return false;
		}
		create_uniform_float(name, value, minValue, maxValue, ud);
	}
	else {
		return false;
	}
	return true;
}

bool parseUniformSampler2D(const std::string line, UniformData &ud) {
	std::size_t found = line.find("uniform");
	if (found == std::string::npos) {
		return false;
	}
	std::string temp_line = line.substr(found + 7); //strip uniform

	found = temp_line.find("sampler2D");
	if (found != std::string::npos) {
		std::string sampler_name;
		std::string valid_chars = "abcdefghijklmnopqrstuvwxyz_-";
		std::size_t start = temp_line.find_first_of(valid_chars, found + 9);
		std::size_t end = temp_line.find_first_of(" ;", start);
		std::string name = temp_line.substr(start, end - start);
	
		std::string file;
		found = temp_line.find("path(");
		if (found == std::string::npos) {
			return false;
		}
		start = found + 5;
		end = temp_line.find_first_of(")", start);
		file = temp_line.substr(start, end - start);

		create_uniform_sampler2D(name, file, ud);
	}
	else {
		return false;
	}
	return true;
}

void load_textures(const std::vector<UniformData>& ud, std::vector<texture_binding>& texture_bindings) {
	texture_bindings.clear();
	GLuint texture_unit = GL_TEXTURE0;
	for (unsigned int i = 0; i < ud.size(); i++) {
		if (ud[i].type == uniformtype::UT_SAMP2D) {
			if (ud[i].name == "input_map") {
				continue; //special case for off screen fb
			}
			texture_binding tb;

			glGenTextures(1, &tb.texture);
			glBindTexture(GL_TEXTURE_2D, tb.texture);
			int x, y, n;
			char* filename = (char*)ud[i].uniform_data;
			unsigned char *data = stbi_load(filename, &x, &y, &n, 3);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			stbi_image_free(data);
			tb.texture_unit = texture_unit;
			texture_unit++; //This cannot be OK.
			tb.sampler2D = ud[i].name;
			texture_bindings.push_back(tb);

		}
	}
}

GLuint loadShaders(const std::string vs, const std::string fs,std::vector<UniformData>& uniforms, std::vector<char> &shader_errors) {
	for (unsigned int i = 0; i < uniforms.size(); i++) {
		delete_uniform_data(uniforms[i]);
	}
	uniforms.clear();

	std::string vertexCode;
	std::string fragmentCode;
	std::string tempLine;

	std::ifstream vtext(vs);
	if (vtext.is_open()) {
		tempLine = "";
		while (getline(vtext, tempLine)) {
			vertexCode += tempLine + "\n";
		}
	}
	std::ifstream ftext(fs);
	if (ftext.is_open()) {
		tempLine = "";
		while (getline(ftext, tempLine)) {
			fragmentCode += tempLine + "\n";
			UniformData ud;
			if (parseUniformFloat(tempLine, ud)) {
				uniforms.push_back(ud);
			}
			else if (parseUniformSampler2D(tempLine, ud)) {
				uniforms.push_back(ud);
			}
		}
	}

	GLuint vsID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fsID = glCreateShader(GL_FRAGMENT_SHADER);
	char const* vcp = vertexCode.c_str();
	glShaderSource(vsID, 1, &vcp, NULL);
	glCompileShader(vsID);

	char const* fcp = fragmentCode.c_str();
	glShaderSource(fsID, 1, &fcp, NULL);
	glCompileShader(fsID);

	GLint Result = GL_FALSE;
	int InfoLogLength;
	glGetShaderiv(fsID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(fsID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0){
		//std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		shader_errors.resize(InfoLogLength + 1);
		glGetShaderInfoLog(fsID, InfoLogLength, NULL, &shader_errors[0]);
		printf("%s\n", &shader_errors[0]);
	}




	GLuint pID = glCreateProgram();
	glAttachShader(pID, vsID);
	glAttachShader(pID, fsID);
	glLinkProgram(pID);
	glDetachShader(pID, vsID);
	glDetachShader(pID, fsID);
	glDeleteShader(vsID);
	glDeleteShader(fsID);
	return pID;

}

GLuint setupBuffer(GLfloat *data, unsigned int data_size) {
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, data_size, data, GL_STATIC_DRAW);
	return buffer;
}

void drawStuff(GLuint vertexbuffer, GLuint uvbuffer) {

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

void setupRenderTexture(unsigned int width, unsigned int height, GLuint &fb, GLuint &rtt) {
	glGenFramebuffers(1, &fb);
	glBindFramebuffer(GL_FRAMEBUFFER, fb);

	glGenTextures(1, &rtt);
	glBindTexture(GL_TEXTURE_2D, rtt);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
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
    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui OpenGL3 example", NULL, NULL);

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
	std::vector<char> shader_errors;
	GLuint pID = loadShaders("d:/shaders/vertex.glsl", "d:/shaders/fragment3.glsl", uniforms, shader_errors);

	std::vector<texture_binding> texture_bindings;
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
	vertexbuffer = setupBuffer(&vertex_data[0], sizeof(vertex_data));
	GLuint uvbuffer;
	uvbuffer = setupBuffer(&uv_data[0], sizeof(uv_data));

#endif
	int display_w, display_h;
	glfwGetFramebufferSize(window, &display_w, &display_h);
	GLuint fb = 0;
	GLuint rtt = 0;

	setupRenderTexture(display_w, display_h, fb, rtt);
	std::vector<UniformData> uniforms_fb;
	std::vector<char> shader_errors_fb;

	GLuint fbpid = loadShaders("d:/shaders/vertex.glsl", "d:/shaders/fragment.glsl", uniforms_fb, shader_errors_fb);
	GLuint fbTexID = glGetUniformLocation(fbpid, "input_map");
	// Main loop

	GLfloat time = 0.0f;

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
				pID = loadShaders("d:/shaders/vertex.glsl", "d:/shaders/fragment3.glsl", uniforms, shader_errors);
				load_textures(uniforms, texture_bindings);
				restart = true;
			}
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::Text(&shader_errors[0]);

		}

        // 2. Show another simple window, this time using an explicit Begin/End pair
		if (uniforms.size() != 0) {
            ImGui::SetNextWindowSize(ImVec2(200,200), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Shader Parameters");
            ImGui::Text("Hello");
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
		//int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);

		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		if (restart) {
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
			uniformID = glGetUniformLocation(pID, texture_bindings[i].sampler2D.c_str());
			glUniform1i(uniformID, i/*texture_bindings[i].texture_unit*/);
			glActiveTexture(texture_bindings[i].texture_unit);
			glBindTexture(GL_TEXTURE_2D, texture_bindings[i].texture);
			
		}

		//special off screen render texture
		uniformID = glGetUniformLocation(pID, "input_map");
		glUniform1i(uniformID, texture_bindings.size()/*texture_bindings[i].texture_unit*/);
		glActiveTexture(texture_bindings[texture_bindings.size()-1].texture_unit+1);
		glBindTexture(GL_TEXTURE_2D, rtt);

		drawStuff(vertexbuffer, uvbuffer);

		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, display_w, display_h);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(fbpid);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, rtt);
		glProgramUniform1i(fbpid, fbTexID, 0);
		drawStuff(vertexbuffer, uvbuffer);
		ImGui::Render();

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();

    return 0;
}
