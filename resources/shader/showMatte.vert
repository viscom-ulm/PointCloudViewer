#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 albedo;
layout(location = 3) in vec3 directIllumination;
layout(location = 4) in vec3 globalIllumination;

uniform mat4 MVP;
uniform int renderType;
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
    if (renderType == 0) vs_out.result = globalIllumination;
    else if (renderType == 1) vs_out.result = albedo;
    else if (renderType == 2) vs_out.result = directIllumination;
}
