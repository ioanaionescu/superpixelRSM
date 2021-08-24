#version 430
#define SUPERPIXELS 607
#define PI 3.1415926538
in vec2 texture_coord;

uniform sampler2D texture_1;

uniform sampler2D flux_texture;
uniform sampler2D normals_texture;
uniform sampler2D positions_texture;

uniform sampler2DShadow depthMap;
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
uniform int superpixels_total;

uniform int switchToSuperpixels;

uniform int row;
uniform int column;


in vec3 world_position;
in vec3 world_normal;
in vec4 FragPosLightSpace;

struct Superpixel
{
	vec4 coord;
	vec4 color;
	vec4 normal;
	vec4 position;
	vec4 weight;
};

layout(std430, binding = 0) buffer superpixels {
	Superpixel data[];
};

layout(location = 0) out vec3 out_color;


vec3 ambientLightAllPixels() 
{
	vec3 ambient = vec3(0);
	vec3 pos = FragPosLightSpace.xyz / FragPosLightSpace.w;
	pos = pos * 0.5 + 0.5;

	vec3 n = normalize(world_normal);
	vec3 x = world_position;
	int height;
	int width;
	//ivec2 size = textureSize(flux_texture, 0);
	//height = size.y;
	//width = size.x;
	for (int i = row * 32 ; i < (row + 1) * 32; i++) 
	{
		for (int j = column * 32; j < (column + 1) * 32; j++) {
			ivec2 coord = ivec2(j, i);
			vec3 flux = texelFetch(flux_texture, coord, 0).rgb;
			vec3 x_p = texelFetch(positions_texture, coord, 0).rgb;
			vec3 n_p = texelFetch(normals_texture, coord, 0).rgb;

			vec3 r = x - x_p;
			
			float d2 = dot(r, r);
			vec3 E_p = flux * (max(0.0, dot(n_p, r)) * max(0.0, dot(n, -r)));
			E_p /= (d2 * d2);

			ambient += E_p;
		}

	}

	return ambient;
}

void main()
{
	//```````````````````````````````````````
	//ambient
	vec3 ambient;

	ambient = ambientLightAllPixels();
	out_color = ambient;

	//float rand = row /32;
	//float coloana = column /32;
	//out_color = vec3(0.0, 1.0, 0.0);
	
	/*if (row == 2) {
		out_color = vec3(0.0, 1.0, 0.0);
	}

	if (row == 3) {
	out_color = vec3(0.0, 1.0, 1.0);
	}
	if (row == 4) {
		out_color = vec3(1.0, 0.0, 0.0);
	}*/


}
