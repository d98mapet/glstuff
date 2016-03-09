#version 330 core
in vec2 uv;

out vec3 color;

uniform sampler2D input_map;
uniform float iGlobalTime;


void main(){
	color = vec3(texture(input_map,uv).rgb);
	
	if (iGlobalTime>0.0) {
		color /= iGlobalTime;
	}
	
}