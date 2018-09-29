#version 450
// This is a HBAO-Shader for OpenGL, based upon nvidias directX implementation
// supplied in their SampleSDK available from nvidia.com
// The slides describing the implementation is available at
// http://www.nvidia.co.uk/object/siggraph-2008-HBAO.html


const float PI = 3.14159265;

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D materialColorTexture;
uniform sampler2D directIlluminationTexture;
uniform vec3 camPos;

const int kernelSize = 10;
const int radiusi = 20;

in vec2 texCoord;

out vec4 ssgi;

void main(void)
{
    vec4 positionMask = texture(positionTexture, texCoord);
    vec3 position = (vec4(positionMask.rgb, 1)).xyz;
    vec3 normal = normalize(texture(normalTexture, texCoord).rgb);
    if (positionMask.a == 0.0f) discard;

    
    vec3 tangent = dFdx(position);
    vec3 binormal = dFdy(position);
    float A = length(cross(tangent, binormal));

    tangent = normalize(tangent);
    binormal = normalize(binormal);

    vec3 Lind = vec3(0.0);
    float rmax = 1.0f, N = 0.0f;
    for (int x = -kernelSize; x <= kernelSize; ++x) {
        for (int y = -kernelSize; y <= kernelSize; ++y) {
            if (x == 0 && y == 0) continue;

            float radius = 10 * radiusi / position.z;

            vec4 pMask = textureOffset(positionTexture, texCoord, ivec2(radius * x, radius * y));
            if (pMask.a == 0.0f) continue;

            vec3 p = pMask.xyz;
            vec3 n = normalize(textureOffset(normalTexture, texCoord, ivec2(radius * x, radius * y)).rgb);

            vec3 dir = p - position;
            float d = max(1, length(dir));
            rmax = max(rmax, length(dir));
            dir = normalize(dir);
            float costhetar = clamp(dot(dir, normal), 0, 1);
            float costhetas = clamp(dot(-dir, n), 0, 1);

            vec3 t = dFdx(p);
            vec3 b = dFdy(p);
            // float As = radius * radius * length(cross(t, b));
            // float As = (radiusi * radiusi) / (kSize * kSize);
            float Afact = clamp(dot(n, normalize(camPos - p)), 0.1, 1.0);
            // As /= Afact;

            vec3 Is = textureOffset(directIlluminationTexture, texCoord, ivec2(radius * x, radius * y)).rgb;
            vec3 Cs = textureOffset(materialColorTexture, texCoord, ivec2(radius * x, radius * y)).rgb;
            Lind += (Is * Cs * costhetas * costhetar) / (Afact * d * d);
            N += 1.0f;
        }
    }

    float As = (rmax * rmax) / N;

    // Lind = vec3(0);
    vec3 I = texture(directIlluminationTexture, texCoord).rgb;
    vec3 C = texture(materialColorTexture, texCoord).rgb;
    vec3 L = (As * Lind) + (I * C) / PI;

    ssgi = vec4(L, 1);
}
