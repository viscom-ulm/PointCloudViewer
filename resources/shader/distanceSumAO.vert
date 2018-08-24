#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in float ao;

uniform mat4 viewProjection;
uniform float pointSize = 1.0f;

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
    vertResult = vec3(ao);
    vertDirectIllumination = vec3(0);
}
