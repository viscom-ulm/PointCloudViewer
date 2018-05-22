#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;

uniform mat4 viewProjection;
uniform mat4 modelMatrix;
uniform mat3 normalMatrix;

out vec3 vertPosition;
out vec3 vertNormal;
out vec2 vertTexCoords;

void main()
{
    vec4 worldPosition = modelMatrix * vec4(position, 1.0);
    vertPosition = position.xyz;
    vertNormal = normalize(normalMatrix * normal);
    vertTexCoords = texCoords;
    gl_Position =  viewProjection * worldPosition;
}
