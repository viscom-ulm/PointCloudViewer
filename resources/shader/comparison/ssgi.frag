#version 450

const float PI = 3.14159265;

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D materialColorTexture;
uniform sampler2D directIlluminationTexture;
uniform vec3 camPos;

const int kernelSize = 15;
const float radiusi = 60.0f;
const float bias = 1.5f;

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

            float radius = radiusi / length(camPos - position);
            vec2 texOffset = texCoord + vec2(radius) * vec2(x, y) / vec2(textureSize(positionTexture, 0).xy);

            vec4 pMask = texture(positionTexture, texOffset);
            if (pMask.a == 0.0f) continue;

            vec3 p = pMask.xyz;
            vec3 n = normalize(texture(normalTexture, texOffset).rgb);

            vec4 IsArea = texture(directIlluminationTexture, texOffset);
            vec3 Is = IsArea.rgb;
            vec3 dir = p - position;
            float d = max(0.0, length(dir));
            rmax = max(rmax, length(dir));
            dir = normalize(dir);
            float costhetar = clamp(dot(dir, normal), 0, 1);
            float costhetas = clamp(dot(-dir, n), 0, 1);

            vec3 t = radius * dFdx(p);
            vec3 b = radius * dFdy(p);
            float A = radius * radius * length(cross(t, b));
            // float As = (radiusi * radiusi) / (kSize * kSize);
            // float A = 1.0f;
            // float Ap = IsArea.a;
            float Ap = A / clamp(dot(n, normalize(camPos - p)), 0.2, 1.0);
            // As /= Afact;

            vec3 Cs = texture(materialColorTexture, texOffset).rgb;

            float G = max(0.0f, (costhetas * costhetar) / (d * d) - bias);
            vec3 f = Cs / PI;

            Lind += clamp(f * G * Ap, 0.0f, 1.0f) * Is; // (Is * Cs * Ap * costhetas * costhetar) / (PI * d * d + Ap);
            N += 1.0f;
        }
    }

    // float As = (rmax * rmax) / N;
    float As = 1.0f * PI / N;
    // float As = 1.0f / N;//2.0f * PI / N;

    // Lind = vec3(0);
    vec3 I = texture(directIlluminationTexture, texCoord).rgb;
    vec3 C = texture(materialColorTexture, texCoord).rgb;
    vec3 L = (As * Lind) + (I * C) / PI;
    L = (As * Lind) * (C / PI);
    L = (As * Lind + I) * (C / PI);

    ssgi = vec4(L, 1);
}
