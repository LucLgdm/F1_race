#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec3 vertexColor;
out vec3 surfaceColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    surfaceColor = vertexColor;
    gl_Position = projection * view * model * vec4(position, 0.0, 1.0);
}
