#version 330 core

uniform sampler2D diffuseTexture;

in vec3 vertPosition;
in vec3 vertNormal;
in vec3 vertTexCoords;

layout(location = 0) out vec4 position;
layout(location = 1) out vec4 normal;
layout(location = 2) out vec4 color;

void main()
{
    position = vec4(vertPosition, 1.0);
    normal = vec4(vertNormal, 1.0);
    color = vec4(texture(diffuseTexture, vertTexCoords).rgb, 1.0);
}
