#version 330 core

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D materialColorTexture;
uniform sampler2D directIlluminationTexture;
uniform sampler2D globalIlluminationTexture;
uniform int isAmbientOcclusion = 0;
uniform int compositionType;

in vec2 texCoord;

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
    vec4 giWeightSum = texture(globalIlluminationTexture, texCoord);
    float weightSum = diWeightSum.a;

    vec3 globalIllumination = giWeightSum.rgb / giWeightSum.a;
    vec3 directIllumination = diWeightSum.rgb / diWeightSum.a;

    vec3 materialColor = texture(materialColorTexture, texCoord).rgb;

    // vec3 L = materialColor * directIllumination + globalIllumination;
    vec3 L = globalIllumination;
    if (compositionType == 0) L = globalIllumination;
    else if (compositionType == 1) L = directIllumination * (materialColor / PI);
    else if (compositionType == 2 && isAmbientOcclusion == 0) L = (directIllumination + globalIllumination) * (materialColor / PI);
    else if (compositionType == 2 && isAmbientOcclusion == 1) L = (directIllumination * globalIllumination) * (materialColor / PI);
    // TODO: if Ambient occlusion
    // L = (directIllumination * globalIllumination) * (materialColor / PI);
    color = vec4(L, 1.0);
}
