#version 330 core

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D materialColorTexture;
uniform sampler2D directIlluminationTexture;
uniform vec3 lightPos = vec3(10.0, 0.0, 0.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform float lightMultiplicator = 1.0;
uniform int renderType = 0;
uniform int compositionType = 0;

in vec2 texCoord;

out vec4 color;

const float PI = 3.1415926535897932384626433832795;

void main()
{
    vec3 position = texture(positionTexture, texCoord).rgb;
    vec4 n4 = texture(normalTexture, texCoord);
    vec3 materialColor = texture(materialColorTexture, texCoord).rgb;
    vec3 directIllumination = texture(directIlluminationTexture, texCoord).rgb;
    
    if (n4.a == 0.0) {
        color = vec4(0,0,0,0);
        return;
    }

    vec3 normal = n4.rgb;

    vec3 light = lightPos - position;

    float nDotL = clamp(dot(normal, light), 0.0, 1.0);
    vec3 C = (materialColor / PI) * directIllumination;

    vec3 gi = vec3(0.0);
    if (renderType == 0) gi = vec3(0.0); // global illuminatiion
    else if (renderType == 1) gi = materialColor; // albedo
    else if (renderType == 2) gi = C; // direct illumination

    vec3 L = gi;
    if (compositionType == 0) L = gi; // pass through
    else if (compositionType == 1) L = directIllumination; // direct only
    else if (compositionType == 2) L = C + gi * (materialColor / PI); // combine all
    // else if (compositionType == 2 && isAmbientOcclusion == 0) L = (directIllumination + globalIllumination) * (materialColor / PI);
    // else if (compositionType == 2 && isAmbientOcclusion == 1) L = (directIllumination * globalIllumination) * (materialColor / PI);
    color = vec4(L, 1.0);
}
