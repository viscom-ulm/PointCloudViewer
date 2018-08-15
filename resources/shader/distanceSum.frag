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
    if (ndot <= 0) discard;

    vec3 vtNormal = normalize(vertNormal);
    vec3 normal = normalize(n4.xyz);
    vec3 position = texture(positionTexture, texCoords).xyz;

    vec3 b = normalize(vec3(0, -1, -1));
    if (vtNormal.x <= vtNormal.y && vtNormal.x <= vtNormal.z) b = vec3(1, 0, 0);
    else if (vtNormal.y <= vtNormal.z) b = vec3(0, 1, 0);
    else b = vec3(0, 0, 1);

    vec3 tangent = normalize(cross(vtNormal, b));
    vec3 binormal = normalize(cross(vtNormal, tangent));
    mat3 lFrame = mat3(vtNormal, tangent, binormal);
    mat3 cM = mat3(vec3(5, 0 , 0), vec3(0, 0.01, 0), vec3(0, 0, 0.01));
    mat3 tInvCM = (lFrame) * cM * transpose(lFrame);

    vec3 vertDiff = position - vertPosition;
    // float dist = dot(vertDiff, vertDiff);

    vec3 cInvDiff = tInvCM * vertDiff;
    float dist = dot(vertDiff, cInvDiff);

    float weight = 1.0 / (pow(dist, distancePower) + 0.00000001);
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
