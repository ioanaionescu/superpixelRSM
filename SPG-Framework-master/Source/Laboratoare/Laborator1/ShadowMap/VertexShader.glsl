#version 330 core
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texture_coord;

uniform mat4 lightSpaceMatrix;
uniform mat4 Model;

void main()
{
    gl_Position = lightSpaceMatrix * Model * vec4(v_position, 1.0);
}