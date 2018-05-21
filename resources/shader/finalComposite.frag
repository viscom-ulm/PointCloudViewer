#version 330 core

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D materialColorTexture;
uniform sampler2D directIlluminationTexture;
uniform sampler2D globalIlluminationTexture;

in vec3 texCoord;

out vec4 color;

void main()
{
    vec4 n4 = texture(normalTexture, texCoords);
    if (n4.a == 0.0) return;

    vec4 diWeightSum = texture(directIlluminationTexture, texCoords);
    float weightSum = diWeightSum.a;

    vec3 globalIllumination = texture(globalIlluminationTexture, texCoords).rgb / weightSum;
    vec3 directIllumination = diWeightSum.rgb / weightSum;

    vec3 materialColor = texture(materialColorTexture, texCoords);

    vec3 L = materialColor * directIllumination + globalIllumination;
    color = vec4(L, 1.0);
}
