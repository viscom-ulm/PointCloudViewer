#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 albedo;
layout(location = 3) in vec3 directIllumination;
layout(location = 4) in vec3 globalIllumination;

uniform mat4 viewProjection;
uniform float bbRadius = 10.0f;

out vec3 vertPosition;
out vec3 vertNormal;
out vec3 vertResult;
out vec3 vertDirectIllumination;

void main()
{
    gl_Position =  viewProjection * vec4(position, 1.0);
    gl_PointSize = bbRadius * 50.0f / gl_Position.w;

    vertPosition = position;
    vertNormal = normal;
    vertResult = globalIllumination;
    vertDirectIllumination = directIllumination;
}
