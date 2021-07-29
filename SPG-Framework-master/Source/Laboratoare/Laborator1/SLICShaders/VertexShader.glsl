#version 430

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texture_coord;

// Uniform properties
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

out vec2 texture_coord;

out vec4 FragPosLightSpace;
uniform mat4 lightSpaceMatrix;

out vec3 world_position;
out vec3 world_normal;

void main()
{

	world_position = (Model * vec4(v_position, 1)).xyz;
	world_normal = normalize(mat3(Model) * normalize(v_normal));

	texture_coord = v_texture_coord;

	//gl_Position = Projection * View * Model * vec4(v_position, 1);
	FragPosLightSpace = lightSpaceMatrix * vec4(world_position, 1.0);
	gl_Position = Projection * View * vec4(world_position, 1);
}


