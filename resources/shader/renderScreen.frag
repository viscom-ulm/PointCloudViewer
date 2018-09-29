#version 330 core

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D materialColorTexture;
uniform sampler2D directIlluminationTexture;
uniform sampler2D globalIlluminationTexture;
uniform vec3 lightPos = vec3(10.0, 0.0, 0.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform float lightMultiplicator = 1.0;
uniform vec3 camPos;
uniform int compositionType = 0;

in vec2 texCoord;

out vec4 color;

const float PI = 3.1415926535897932384626433832795;

void main()
{
    vec3 position = texture(positionTexture, texCoord).rgb;
    vec3 normal = texture(normalTexture, texCoord).rgb;
    vec3 albedo = texture(materialColorTexture, texCoord).rgb;
    vec3 directIllumination = texture(directIlluminationTexture, texCoord).rgb;
    vec3 result = texture(globalIlluminationTexture, texCoord).rgb;

    if (dot(normal, normal) < 0.1) discard;

    vec3 L = vec3(0);
    if (compositionType == 0) L = result;
    else if (compositionType == 1) L = directIllumination * (albedo / PI);
    else if (compositionType == 2) L = (directIllumination + result) * (albedo / PI);
    else if (compositionType == 3) L = (directIllumination * result) * (albedo / PI);

    color = vec4(L, 1.0f);
}
