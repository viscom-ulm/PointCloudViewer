#version 450

const float PI = 3.14159265;

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D materialColorTexture;
uniform sampler2D directIlluminationTexture;
uniform vec3 camPos;

const int kernelSize = 5;
const int radiusi = 5;

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

            float radius = radiusi;// / position.z;

            vec4 pMask = textureOffset(positionTexture, texCoord, ivec2(radius * x, radius * y));
            if (pMask.a == 0.0f) continue;

            vec3 p = pMask.xyz;
            vec3 n = normalize(textureOffset(normalTexture, texCoord, ivec2(radius * x, radius * y)).rgb);

            vec4 IsArea = textureOffset(directIlluminationTexture, texCoord, ivec2(radius * x, radius * y));
            vec3 Is = IsArea.rgb;
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
            float A = 1.0f;
            // float Ap = IsArea.a;
            float Ap = A / clamp(dot(n, normalize(camPos - p)), 0.2, 1.0);
            // As /= Afact;

            vec3 Cs = textureOffset(materialColorTexture, texCoord, ivec2(radius * x, radius * y)).rgb;
            Lind += (Is * Cs * Ap * costhetas * costhetar) / (PI * d * d + Ap);
            N += 1.0f;
        }
    }

    // float As = (rmax * rmax) / N;
    float As = 2.0f * PI / N;

    // Lind = vec3(0);
    vec3 I = texture(directIlluminationTexture, texCoord).rgb;
    vec3 C = texture(materialColorTexture, texCoord).rgb;
    vec3 L = (As * Lind) + (I * C) / PI;
    // L = (As * Lind);

    ssgi = vec4(L, 1);
}
