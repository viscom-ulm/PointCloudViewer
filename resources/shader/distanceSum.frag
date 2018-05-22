#version 330 core

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform float distancePower = 1.0;

in vec3 vertPosition;
in vec3 vertNormal;
in vec3 vertResult;
in vec3 vertDirectIllumination;

layout(location = 0) out vec4 directIllumination;
layout(location = 1) out vec4 globalIllumination;

void main()
{
    vec2 texCoords = gl_FragCoord.xy / vec2(textureSize(positionTexture, 0));
    vec4 n4 = texture(normalTexture, texCoords);
    if (n4.a == 0.0) return;
    if (dot(n4.xyz, vertNormal) < 0.0) return;

    vec3 normal = normalize(n4.xyz);
    vec3 position = texture(positionTexture, texCoords).xyz;

    float weight = 1.0 / pow(distance(position, vertPosition), distancePower);
    directIllumination = vec4(weight * vertDirectIllumination, weight);
    globalIllumination = vec4(weight * vertResult, 1.0);
}
