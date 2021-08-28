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


vec3 ambientLightAllPixelsWRONG() // varianta cu texelFetch care da erroare
{
	vec3 ambient = vec3(0);
	vec3 pos = FragPosLightSpace.xyz / FragPosLightSpace.w;
	pos = pos * 0.5 + 0.5;

	vec3 n = normalize(world_normal);
	vec3 x = world_position;
	int height;
	int width;
	ivec2 size = textureSize(flux_texture, 0);
	height = size.y;
	width = size.x;
	int row = 31;
	for (int i = (row + 0) * 32; i < (row + 1) * 32; i++) // adca in loc de 100 e height/width primesc eroare de 
	{
		for (int j = (row + 0) * 32; j < (row + 1) * 32; j++) {
			ivec2 coord = ivec2(j, i);
			vec3 flux = texelFetch(flux_texture, coord , 0).rgb;
			vec3 x_p = texelFetch(positions_texture, coord , 0).rgb;
			vec3 n_p = texelFetch(normals_texture,coord , 0).rgb;

			vec3 r = x - x_p;
			float d2 = dot(r, r);
			vec3 E_p = flux * (max(0.0, dot(n_p, r)) * max(0.0, dot(n, -r)));
			E_p /= (d2 * d2);

			ambient += E_p;
		}
		
	}

	return ambient;
}

vec3 ambientLightAllPixels() 
{
	vec3 ambient = vec3(0);
	vec3 pos = FragPosLightSpace.xyz / FragPosLightSpace.w;
	pos = pos * 0.5 + 0.5;

	vec3 n = normalize(world_normal);
	vec3 x = world_position;
	float height = 720.0;
	float width = 1280.0;
	//vec2 size = textureSize(flux_texture, 0);
	//height = float(size.y);
	//width = float(size.x);
	for (float i = 0; i < 1; i += 0.01) // += 1/720 1/height
	{
		for (float j = 0; j < 1; j += 0.01) { //+=1/1280  1/width
			vec2 coord = vec2(i, j);
			vec3 flux = texture(flux_texture, coord).rgb;
			vec3 x_p = texture(positions_texture, coord).rgb;
			vec3 n_p = texture(normals_texture, coord).rgb;

			vec3 r = x - x_p;
			float d2 = dot(r, r);
			vec3 E_p = flux * (max(0.0, dot(n_p, r)) * max(0.0, dot(n, -r)));
			E_p /= (d2 * d2);

			ambient += E_p;
		}

	}

	return ambient;
}

vec3 ambientLight() //RSM
{
	vec3 ambient = vec3(0);
	vec3 pos = FragPosLightSpace.xyz / FragPosLightSpace.w;
	pos = pos * 0.5 + 0.5; 

	vec3 n = normalize(world_normal);
	vec3 x = world_position;
	for (int i = 0; i < superpixels_total; i++)
	{
		float pondere = data[i].weight.x *  20;
		vec3 flux = data[i].color.xyz; // 
		vec3 x_p = data[i].position.xyz;
		vec3 n_p = data[i].normal.xyz;

		vec3 r = x - x_p;								
		float d2 = dot(r, r);							
		vec3 E_p = flux * (max(0.0, dot(n_p, r)) * max(0.0, dot(n, -r)));
		E_p /= (d2 * d2);				

		ambient += E_p * pondere;
	}

	return ambient ;					
}

vec3 ambientLight2() //Aria
{
	vec3 ambient = vec3(0);
	vec3 pos = FragPosLightSpace.xyz / FragPosLightSpace.w;
	pos = pos * 0.5 + 0.5;

	vec3 n = normalize(world_normal);
	vec3 x = world_position;
	//float Adisk = 1 * 1 * PI;
	for (int i = 0; i < superpixels_total; i++)
	{
		float Adisk = data[i].weight.x * 1000;
		vec3 flux = data[i].color.xyz; // 
		vec3 x_p = data[i].position.xyz;
		vec3 n_p = data[i].normal.xyz;

		vec3 r = x - x_p;
		float d2 = dot(r, r);
		float cos_0x = dot(n_p, r) / (length(n_p) * length(r));
		float cos_0xc = dot(n, -r) / (length(n) * length(-r));
		vec3 E_p = flux * (Adisk * cos_0x * cos_0xc) / (Adisk + PI * d2);
		
		ambient += E_p ;
	}

	return ambient;
}

float shadowCalc() 
{
	//vec3 color_texture = texture(texture_1, texture_coord).rgb;
	vec3 pos = FragPosLightSpace.xyz / FragPosLightSpace.w;
	pos = pos * 0.5 + 0.5;
	//pos = pos / pos.w;

	
	float bias = 0.001;

	// PCF
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(depthMap, 0);
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			vec3 checkPos = vec3(pos.xy + vec2(x, y) * texelSize, pos.z - bias);
			float depth = texture(depthMap, checkPos, 0);

			shadow += depth;
		}
	}
	return shadow / 9.0;
	//return (depth + bias) < pos.z ? 0.0 : 1.0;
}
void main()
{

	// Compute world space vectors
	vec3 N =  normalize(world_normal);

	vec3 V = normalize(eye_position - world_position);
	vec3 L = normalize(light_position - world_position);
	vec3 H = normalize(L + V);
	vec3 R = normalize(reflect(-L, N));

	// Define ambient light component
	float ambient_light = material_kd * 0.5f * diffuse.r;

	// Compute diffuse light component
	float diffuse_light = material_kd * max(dot(N, L), 0);
	// Compute specular light component
	float specular_light = 0;

	if (diffuse_light > 0)
	{
		// specular_light = material_ks * pow(max(dot(world_normal, H), 0), material_shininess);
		specular_light = material_ks * pow(max(dot(V, R), 0), material_shininess);
	}

	//Shadow
	float shadow = shadowCalc();

	// Compute light
	float d = distance(light_position, world_position);
	float attenuation_factor = 1.0 /(d * d + 1) * 30;
	//attenuation_factor = 1;
	float light = ambient_light + shadow * attenuation_factor * (diffuse_light + specular_light);

	// Send color light output to fragment shader
	vec3 color_texture = texture(texture_1, texture_coord).rgb;

	//out_color = color_texture * diffuse;

	out_color = color_texture * light *diffuse;

	

	//```````````````````````````````````````
	//ambient
	vec3 ambient;
	ambient = ambientLight();
	//switch (switchToSuperpixels) {
	//case 1: // RSM 
	//	ambient *= 0.17;
	//	break;
	//case 2: // RSMC -> cu aria
	//	ambient *= 1;
	//	break;
	//case 3: // exaluare toti pixeli texelFetch
	//	ambient *= 5;
	//	break;
	//default: // exaluare toti pixeli texture 
	//	ambient *= 10;
	//}
 



	//diffuse
	vec3 lightColor = vec3(1.0);
	vec3 diffuse = diffuse_light * lightColor;

	//specular
	vec3 specular = specular_light * lightColor;

	vec3 lighting = (ambient  + shadow * (diffuse + specular)) * color_texture ;

	//out_color = lighting;

	out_color = ambient * 5;



}
