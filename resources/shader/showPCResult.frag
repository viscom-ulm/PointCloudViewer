#version 330 core

in vec3 vertPosition;
in vec3 vertNormal;
in vec3 vertResult;

out vec4 color;

void main()
{
    color = vec4(vertResult, 1.0);
}
