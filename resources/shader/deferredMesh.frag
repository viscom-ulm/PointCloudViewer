#version 330 core

uniform sampler2D diffuseTexture;
uniform vec3 camPos;
uniform vec3 sigmat;
uniform float eta;
uniform vec3 lightPos = vec3(10.0, 0.0, 0.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform float lightMultiplicator = 1.0;
uniform int outputDirectLight = 1;

in vec3 vertPosition;
in vec3 vertNormal;
in vec2 vertTexCoords;

layout(location = 0) out vec4 position;
layout(location = 1) out vec4 normal;
layout(location = 2) out vec4 color;
layout(location = 3) out vec4 scatteringParams;
layout(location = 4) out vec4 directIllumination;

void main()
{
    if (dot(vertNormal, camPos - vertPosition) < 0) discard;
    
    position = vec4(vertPosition, 1.0);
    normal = vec4(normalize(vertNormal), 1.0);
    color = vec4(texture(diffuseTexture, vertTexCoords).rgb, 1.0);
    scatteringParams = vec4(sigmat, eta);

    vec3 light = lightPos - position.xyz;

    float nDotL = clamp(dot(normal.xyz, light), 0.0, 1.0);
    if (outputDirectLight == 1) directIllumination = vec4(nDotL * lightColor * lightMultiplicator, 1.0);
}
