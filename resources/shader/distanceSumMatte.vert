#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 albedo;
layout(location = 3) in vec3 directIllumination;
layout(location = 4) in vec3 globalIllumination;

uniform mat4 viewProjection;
uniform float pointSize = 1.0f;
uniform int renderType;

out vec3 vertPosition;
out vec3 vertNormal;
out vec3 vertResult;
out vec3 vertDirectIllumination;

void main()
{
    gl_Position =  viewProjection * vec4(position, 1.0);
    gl_PointSize = pointSize * 750.0f / gl_Position.w;

    vertPosition = position;
    vertNormal = normal;
    if (renderType == 0) vertResult = globalIllumination;
    else if (renderType == 1) vertResult = albedo;
    else if (renderType == 2) vertResult = directIllumination;
    vertDirectIllumination = directIllumination;
}
