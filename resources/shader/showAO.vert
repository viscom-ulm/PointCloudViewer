#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in float ao;

uniform mat4 MVP;
uniform float bbRadius;

out vec3 vertPosition;
out vec3 vertNormal;
out vec3 vertResult;

void main()
{
    gl_Position =  MVP * vec4(position, 1.0);
    gl_PointSize = bbRadius * 50.0f / gl_Position.w;

    vertPosition = position;
    vertNormal = normal;
    vertResult = vec3(ao);
}
