#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in float ao;

uniform mat4 MVP;
uniform float bbRadius;

out VS_OUT {
    vec3 position;
    vec3 normal;
    vec3 result;
} vs_out;

void main()
{
    gl_Position =  MVP * vec4(position, 1.0);
    gl_PointSize = bbRadius * 50.0f / gl_Position.w;

    vs_out.position = position;
    vs_out.normal = normal;
    vs_out.result = vec3(ao);
}
