#version 330 core

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D materialColorTexture;
uniform sampler2D directIlluminationTexture;
uniform sampler2D globalIlluminationTexture;

in vec2 texCoord;
uniform int compositionType;

out vec4 color;

const float PI = 3.1415926535897932384626433832795;

void main()
{
    vec4 n4 = texture(normalTexture, texCoord);
    if (n4.a == 0.0) {
        color = vec4(0,0,0,0);
        return;
    }

    vec4 diWeightSum = texture(directIlluminationTexture, texCoord);
    float weightSum = diWeightSum.a;

    vec3 globalIllumination = texture(globalIlluminationTexture, texCoord).rgb / weightSum;
    vec3 directIllumination = diWeightSum.rgb / weightSum;

    vec3 materialColor = texture(materialColorTexture, texCoord).rgb;

    // vec3 L = materialColor * directIllumination + globalIllumination;
    vec3 L = globalIllumination;
    if (compositionType == 0) L = globalIllumination;
    else if (compositionType == 1) L = directIllumination * (materialColor / PI);
    else if (compositionType == 2) L = (directIllumination + globalIllumination) * ( materialColor / PI);
    color = vec4(0.7 * L, 1.0);
}
