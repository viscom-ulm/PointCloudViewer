#version 450

const float PI = 3.14159265;

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D materialColorTexture;
uniform sampler2D sssIlluminationTexture;
uniform sampler2D kernelTexture; // params.kernelTexture
uniform vec3 camPos;
uniform float maxOffsetMm; // params.maxOffsetMMTex = max offset in mm

uniform vec2 dir; // glm::vec2(1.0f / aspectRatio, 1.0f)
uniform float distanceToProjectionWindow; // distanceToProjectionWindow = 0.5f / glm::tan(0.5f * camera.GetFOV());

const int kernelSize = 21;
const float toKernelSizeFactor = 1000.0f;

in vec2 texCoord;

out vec4 sssss;

void calculateOffsets(out float offsets[21]) {
    const float EXPONENT = 2.0f;

    float step = 2.0f * maxOffsetMm / (kernelSize - 1.0f);

    for (int i = 0; i < kernelSize; i++) {
        float o = -maxOffsetMm + float(i) * step;
        float sign = o < 0.0f? -1.0f : 1.0f;
        offsets[i] = maxOffsetMm * sign * abs(pow(o, EXPONENT)) / pow(maxOffsetMm, EXPONENT);
    }
}

void calculateAreas(out float areas[22], float offsets[21]) {
    float sum = 0;

    for (int i = 0; i < kernelSize; i++)
    {
        float w0 = 0; if(i > 0) {w0 = abs(offsets[i] - offsets[i - 1]);}
        float w1 = 0; if(i < 21 - 1) {w1 = abs(offsets[i] - offsets[i + 1]);}
        areas[i] = (w0 + w1) / 2.0f;
        sum += areas[i];
    }
    areas[21] = sum;
}

void main(void)
{
    vec4 positionMask = texture(positionTexture, texCoord);
    vec3 position = (vec4(positionMask.rgb, 1)).xyz;
    vec3 normal = normalize(texture(normalTexture, texCoord).rgb);
    if (positionMask.a == 0.0f) discard;

    vec4 colorM = texture(sssIlluminationTexture, texCoord);
    float depthM = length(camPos - position);
    float scale = distanceToProjectionWindow / depthM;
    vec2 finalStep = (scale * dir) / (toKernelSizeFactor);

    float offsets[21];
    calculateOffsets(offsets);
    float areas[22];
    calculateAreas(areas, offsets);

    vec4 Lsss = vec4(0.0);
    vec4 sum = vec4(0.0) ;

    for (int x = 0; x <= kernelSize; ++x) {
        for (int y = 0; y <= kernelSize; ++y) {
            vec2 kernelOffset = vec2(offsets[x], offsets[y]);
            vec2 uv = (kernelOffset / (2.0f * maxOffsetMm)) + vec2(0.5f);
            vec4 kernelWeight = vec4(texture(kernelTexture, uv).rgb, 1.0f);
            kernelWeight.a = (1.0f / 3.0f) * (kernelWeight.r + kernelWeight.g + kernelWeight.b);
            kernelWeight *= areas[x] * areas[y];

            vec2 offset = texCoord + kernelOffset * finalStep;
            vec4 color = texture(sssIlluminationTexture, offset);
            float depth = length(camPos - texture(positionTexture, offset).xyz);
            
            float s = abs(depthM - depth) * toKernelSizeFactor / (maxOffsetMm);
            s = clamp(s, 0.0f, 1.0f);
            s = min(1.0f, s*1.5f);

            color = mix(color, colorM, s);

            sum += kernelWeight;
            Lsss += kernelWeight * color;
        }
    }

    sssss = Lsss / sum;
    // float As = (rmax * rmax) / N;

    // Lind = vec3(0);
    // vec3 I = texture(directIlluminationTexture, texCoord).rgb;
    // vec3 C = texture(materialColorTexture, texCoord).rgb;
    // vec3 L = (As * Lind) + (I * C) / PI;

    // ssgi = vec4(L, 1);
}
