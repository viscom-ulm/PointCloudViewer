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
    float ndot = dot(n4.xyz, vertNormal);
    if (ndot <= -0.2) discard;

    vec3 normal = normalize(n4.xyz);
    vec3 position = texture(positionTexture, texCoords).xyz;

    vec2 pointPos = (gl_PointCoord.xy - 0.5f) * 2.0f;
    float pointLen2 = 0.0f * dot(pointPos, pointPos);
    vec3 vertPos = position - vertPosition;
    float vertLen2 = 1.0f * dot(vertPos, vertPos);
    // float weight = 1.0 / (pow(distance(position, vertPosition), distancePower) + 0.0001);
    // float weight = 1.0 / (pow(length(pointPos), distancePower) + 0.00001);
    float weight = 1.0 / (pow((pointLen2 + vertLen2), distancePower) + 0.00000001);
    directIllumination = vec4(weight * vertDirectIllumination, weight);
    // directIllumination = vec4(vertPosition, 1.0);
    globalIllumination = vec4(weight * vertResult, 1.0);
    // globalIllumination = vec4(position, 1.0);

    // directIllumination = vec4(weight * vertDirectIllumination, 1.0f);
    // globalIllumination = vec4(vec3(distance(position, vertPosition)), 1.0);
    // globalIllumination = vec4(3 * abs(position), 1.0);
    // globalIllumination = vec4(abs(normal), 1.0);
    // globalIllumination = vec4(vertPosition, 1.0);
}
