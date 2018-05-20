#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 albedo;
layout(location = 3) in vec3 sigma_tp;
layout(location = 4) in vec3 eta;
layout(location = 5) in vec3 directIllumination;
layout(location = 6) in vec3 globalIllumination;

uniform mat4 MVP;
uniform int renderType;

out vec3 vertPosition;
out vec3 vertNormal;
out vec3 vertResult;

void main()
{
    gl_Position =  MVP * vec4(0.1f * position, 1.0);
    gl_PointSize = 5.0f;

    vertPosition = 0.1f * position;
    vertNormal = normal;
    if (renderType == 0) vertResult = globalIllumination;
    else if (renderType == 1) vertResult = albedo;
    else if (renderType == 2) vertResult = sigma_tp;
    else if (renderType == 3) vertResult = eta;
    else if (renderType == 4) vertResult = directIllumination;
}
