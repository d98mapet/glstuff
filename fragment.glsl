#version 330 core
in vec2 uv;

out vec3 color;

uniform sampler2D input_map;
uniform float iGlobalTime;


void main(){
	color = vec3(texture(input_map,uv).rgb);
	//color = vec3(sin(iGlobalTime));
	
	
	if (iGlobalTime>0.0) {
		//color = vec3(cos(iGlobalTime));
		color /= iGlobalTime;
	}
	color = pow( clamp(color,0.0,1.0), vec3(0.45) );
	
	
	
}