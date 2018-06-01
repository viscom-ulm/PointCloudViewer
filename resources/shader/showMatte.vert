#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 albedo;
layout(location = 3) in vec3 directIllumination;
layout(location = 4) in vec3 globalIllumination;

uniform mat4 MVP;
uniform int renderType;
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
    if (renderType == 0) vertResult = 2.0f * globalIllumination;
    else if (renderType == 1) vertResult = albedo;
    else if (renderType == 2) vertResult = directIllumination;
}
