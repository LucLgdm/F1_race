#version 330 core

out vec4 color;
in vec3 surfaceColor;

uniform vec3 objectColor;
uniform bool useVertexColor;

void main()
{
    color = vec4(useVertexColor ? surfaceColor : objectColor, 1.0);
}