#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in float ao;

uniform mat4 MVP;

out vec3 vertPosition;
out vec3 vertNormal;
out float vertAO;

void main()
{
    gl_Position =  MVP * vec4(0.1f * position, 1.0);
    gl_PointSize = 5.0f;

    vertPosition = 0.1f * position;
    vertNormal = normal;
    vertAO = ao;
}

