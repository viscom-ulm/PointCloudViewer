#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 albedo;
layout(location = 3) in vec3 sigma_tp;
layout(location = 4) in vec3 eta;
layout(location = 5) in vec3 directIllumination;
layout(location = 6) in vec3 globalIllumination;

uniform mat4 viewProjection;
uniform mat4 modelMatrix;
uniform float pointSize = 1.0f;

out vec3 vertPosition;
out vec3 vertNormal;
out vec3 vertResult;
out vec3 vertDirectIllumination;

void main()
{
    vec4 wPos = modelMatrix * vec4(position, 1.0);
    gl_Position =  viewProjection * wPos;
    gl_PointSize = pointSize * 750.0f / gl_Position.w;

    vertPosition = wPos.xyz;
    vertNormal = normal;
    vertResult = globalIllumination;
    vertDirectIllumination = directIllumination;
}
