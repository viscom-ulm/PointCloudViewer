#version 430 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;

struct PointCloudOut {
    vec4 position;
    vec4 normal;
    vec4 albedo;
    vec4 scatteringParams;
    vec4 directIllumination;
    vec4 dummy;
};

uniform mat4 viewProjection;
uniform mat4 modelMatrix;
uniform mat3 normalMatrix;
uniform int doExport = 0;

uniform sampler2D diffuseTexture;
uniform vec3 sigmat;
uniform float eta;
uniform vec3 lightPos = vec3(10.0, 0.0, 0.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform float lightMultiplicator = 1.0;

layout(std430) buffer pointCloudOutput {
    PointCloudOut pointCloud[];
};

out vec3 vertPosition;
out vec3 vertNormal;
out vec2 vertTexCoords;

void main()
{
    vec4 worldPosition = modelMatrix * vec4(position, 1.0);
    vertPosition = worldPosition.xyz;
    vertNormal = normalize(normalMatrix * normal);
    vertTexCoords = texCoords;

    if (doExport == 1) {
        pointCloud[gl_VertexID].position = vec4(vertPosition, 1);
        pointCloud[gl_VertexID].normal = vec4(vertNormal, 1);
        pointCloud[gl_VertexID].albedo = vec4(texture(diffuseTexture, vertTexCoords).rgb, 1.0);
        pointCloud[gl_VertexID].scatteringParams = vec4(sigmat, eta);

        vec3 light = lightPos - position.xyz;
        float nDotL = clamp(dot(normal.xyz, light), 0.0, 1.0);
        pointCloud[gl_VertexID].directIllumination = vec4(nDotL * lightColor * lightMultiplicator, 1.0);
    }

    gl_Position =  viewProjection * worldPosition;
}
