#version 430

in vec2 texture_coord;

uniform sampler2D texture_1;
uniform sampler2D depthMap;

uniform vec3 diffuse;

// Uniform properties
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

// Uniforms for light properties
uniform vec3 light_position;
uniform vec3 eye_position;
uniform float material_kd;
uniform float material_ks;
uniform int material_shininess;

in vec3 world_position;
in vec3 world_normal;
in vec4 FragPosLightSpace;

layout(location = 0) out vec3 gColor;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec3 gPosition;


void main()
{

	// Compute world space vectors
	vec3 N =  normalize(world_normal);

	// Send color light output to fragment shader
	vec3 color_texture = texture(texture_1, texture_coord).rgb;

	//out_color = color_texture * diffuse;
	//vec3 w = world_position;
	//out_color = vec3(light) ;
	gColor = color_texture * diffuse;
	gNormal = N;
	gPosition = world_position;
	//out_color = color_texture ;

	
}
