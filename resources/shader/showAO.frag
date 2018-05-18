#version 330 core

in vec3 vertPosition;
in vec3 vertNormal;
in float vertAO;

out vec4 color;

void main()
{
    color = vec4(vertAO, vertAO, vertAO, 1.0);
}
