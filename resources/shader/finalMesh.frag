#version 330 core

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D materialColorTexture;
uniform vec3 lightPos = vec3(10.0, 0.0, 0.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform float lightMultiplicator = 1.0;

in vec2 texCoord;

out vec4 color;

const float PI = 3.1415926535897932384626433832795;

void main()
{
    vec3 position = texture(positionTexture, texCoord).rgb;
    vec4 n4 = texture(normalTexture, texCoord);
    vec3 materialColor = texture(materialColorTexture, texCoord).rgb;
    
    if (n4.a == 0.0) {
        color = vec4(0,0,0,0);
        return;
    }

    vec3 normal = n4.rgb;

    vec3 light = lightPos - position;

    float nDotL = clamp(dot(normal, light), 0.0, 1.0);
    vec3 C = (materialColor / PI) * nDotL * lightColor * lightMultiplicator;
    color = vec4(C, 1.0);
}