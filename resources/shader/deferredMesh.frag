#version 330 core

uniform sampler2D diffuseTexture;
uniform sampler2DShadow shadowMap;
uniform vec3 camPos;
uniform vec3 albedo;
uniform vec3 sigmat;
uniform float eta;
uniform vec3 lightPos = vec3(10.0, 0.0, 0.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform float lightMultiplicator = 1.0;
uniform int outputDirectLight = 1;

const int shadowWidth = 0;

in vec3 vertPosition;
in vec3 vertNormal;
in vec2 vertTexCoords;
in vec4 shadowCoords;

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
    color = vec4(albedo, 1.0);
    scatteringParams = vec4(sigmat, eta);

    vec3 light = normalize(lightPos - position.xyz);

    float nDotL = clamp(dot(normal.xyz, light), 0.0, 1.0);
    if (outputDirectLight == 1) {
        float bias = -2*clamp(0.005*tan(acos(nDotL)), 0, 0.02);
        float visibility = 0.0f, cnt = 0.0f;
        vec3 coords = shadowCoords.xyz / shadowCoords.w;
        coords.z += bias;
        for (int x = -shadowWidth; x <= shadowWidth; ++x) {
            for (int y = -shadowWidth; y <= shadowWidth; ++y) {
                visibility += textureOffset(shadowMap, coords, ivec2(x,y));
                cnt += 1.0f;
            }
        }

        visibility /= cnt;
        
        // float visibility = texture(shadowMap, coords);
        // if (textureProj(shadowMap, shadowCoords.xyw).x < (shadowCoords.z) / shadowCoords.w) visibility = 0.0;

        vec3 t = dFdx(position.xyz);
        vec3 b = dFdy(position.xyz);
        float d = length(lightPos - position.xyz);
        // directIllumination = vec4(visibility * nDotL * lightColor * lightMultiplicator / (d * d), length(cross(t, b)));
        directIllumination = vec4(visibility * nDotL * lightColor * lightMultiplicator / (d * d), 1.0f);
        // directIllumination = vec4(textureProj(shadowMap, shadowCoords.xyw).xyz, (shadowCoords.z) / shadowCoords.w);
    }
}
