#version 450
// This is a HBAO-Shader for OpenGL, based upon nvidias directX implementation
// supplied in their SampleSDK available from nvidia.com
// The slides describing the implementation is available at
// http://www.nvidia.co.uk/object/siggraph-2008-HBAO.html


const float PI = 3.14159265;

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D directIlluminationTexture;
uniform sampler2D noiseTexture;
uniform vec3 camPos;
uniform mat4 view;

const float aoRadius = 0.8;
const float radius2 = aoRadius * aoRadius;
const float maxRadiusPixels = 150.0;
const float AOStrength = 2.0;

const float TanBias = tan(30.0 * PI / 180.0);

const float numDirections = 16.0f;
const float numSamples = 32.0;

in vec2 texCoord;

out vec4 ao;

float TanToSin(float x)
{
    return x * inversesqrt(x*x + 1.0);
}

float InvLength(vec2 V)
{
    return inversesqrt(dot(V,V));
}

float Tangent(vec3 V)
{
    return V.z * InvLength(V.xy);
}

float BiasedTangent(vec3 V)
{
    return V.z * InvLength(V.xy) + TanBias;
}

float Tangent(vec3 P, vec3 S)
{
    return -(P.z - S.z) * InvLength(S.xy - P.xy);
}

// vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)
// {
//    vec3 V1 = Pr - P;
//    vec3 V2 = P - Pl;
//    return (Length2(V1) < Length2(V2)) ? V1 : V2;
// }

vec2 SnapUVOffset(vec2 uv)
{
    return round(uv * vec2(textureSize(positionTexture, 0).xy)) / vec2(textureSize(positionTexture, 0).xy);
}

float Falloff(float d2)
{
    return d2 * (-1.0 / radius2) + 1.0f;
}

float HorizonOcclusion(vec2 deltaUV, vec3 P, vec3 dPdu, vec3 dPdv, float randstep, float numSamples) {
    float aoOut = 0;

    // Offset the first coord with some noise
    vec2 uv = texCoord;// + deltaUV;// + SnapUVOffset(randstep*deltaUV);
    deltaUV = SnapUVOffset(deltaUV);

    // Calculate the tangent vector
    vec3 T = deltaUV.x * dPdu + deltaUV.y * dPdv;

    // Get the angle of the tangent vector from the viewspace axis
    float tanH = BiasedTangent(T);
    float sinH = TanToSin(tanH);

    float tanS;
    float d2;
    vec3 S;

    // Sample to find the maximum angle
    for(float s = 1; s <= numSamples; ++s) {
        uv += deltaUV;
        vec3 S = texture(positionTexture, uv).rgb;
        float tanS = Tangent(P, S);
        float d2 = dot(S - P, S - P);

        // Is the sample within the radius and the angle greater?
        if(d2 < radius2 && tanS > tanH) {
            float sinS = TanToSin(tanS);
            // Apply falloff based on the distance
            aoOut += Falloff(d2) * (sinS - sinH);

            tanH = tanS;
            sinH = sinS;
        }
    }

    return aoOut;
}

float HBAO(vec2 deltaUV, vec3 pos, vec3 n, float randstep, float numSamples) {
    float aoOut = 0;

    // Offset the first coord with some noise
    vec2 uv = texCoord + SnapUVOffset(randstep*deltaUV);
    deltaUV = SnapUVOffset(deltaUV);

    for(float s = 1; s <= numSamples; ++s) {
        uv += deltaUV;
        vec4 S = texture(positionTexture, uv);
        if (S.a == 0.0) continue;

        // vec3 V = (view * S).xyz - pos;
        vec3 V = S.xyz - pos;
        float VdotV = dot(V, V);
        float NdotV = dot(n, V) * 1.0/sqrt(VdotV);

        aoOut += clamp(NdotV - 0.1,0,1) * clamp(Falloff(VdotV),0,1);
    }
    return aoOut;
}

float HBAO2(vec2 deltaUV, vec3 pos, vec3 viewDir, vec3 n, vec3 t, float randstep, float numSamples) {
    float aoOut = 0;

    // Offset the first coord with some noise
    vec2 uv = texCoord + SnapUVOffset(randstep*deltaUV);
    deltaUV = SnapUVOffset(deltaUV);

    // float tT = acos(dot(n, viewDir));
    float tT = atan(t.z / length(t.xy));

    float hHmax = tT, r = 0.0, rmax = 0.0;
    for(float s = 1; s <= numSamples; ++s) {
        uv += deltaUV;
        vec4 S = texture(positionTexture, uv);
        if (S.a == 0.0) continue;

        vec3 H = (view * S).xyz - pos;
        float lenH = dot(H, H);
        H = normalize(H);
        // cos(x + p/2) = -sin(x)
        // cos(p/2 - x) = cos(-x + p/2) = -sin(-x) = sin(x)
        // float hH = asin(dot(H, viewDir));

        float hH = atan(H.z / length(H.xy));
        // vec3 a = cross(cross(H, viewDir), viewDir);
        // if (dot(a,n) < 0) hH = acos(dot(a, H));
        // else hH = acos(-dot(a, H));


        // vec3 D = S - pos;
        // float alpha = atan(-length(D - camPos) / length(D.xy));
        if (abs(hH) > abs(hHmax)) {
            hHmax = hH;
            r = lenH;
        }
        rmax = max(rmax, lenH);
    }

    // return 1 * (sinHmax - sinT) * max(0.0, 1.0 - (r / rmax));
    return 1*(sin(hHmax) - sin(tT));
}

void ComputeSteps(inout vec2 stepSizeUv, inout float numSteps, float rayRadiusPix, float rand)
{
    // Avoid oversampling if numSteps is greater than the kernel radius in pixels
    numSteps = min(numSamples, rayRadiusPix);

    // Divide by Ns+1 so that the farthest samples are not fully attenuated
    float stepSizePix = rayRadiusPix / (numSteps + 1);

    // Clamp numSteps if it is greater than the max kernel footprint
    float maxNumSteps = maxRadiusPixels / stepSizePix;
    if (maxNumSteps < numSteps)
    {
        // Use dithering to avoid AO discontinuities
        numSteps = floor(maxNumSteps + rand);
        numSteps = max(numSteps, 1);
        stepSizePix = maxRadiusPixels / numSteps;
    }

    // Step size in uv space
    stepSizeUv = stepSizePix / vec2(textureSize(positionTexture, 0).xy);
}

void main(void)
{
    vec4 positionMask = texture(positionTexture, texCoord);
    // vec3 position = (view * vec4(positionMask.rgb, 1)).xyz;
    vec3 position = (vec4(positionMask.rgb, 1)).xyz;
    vec3 normal = normalize(texture(normalTexture, texCoord).rgb);
    if (positionMask.a == 0.0f) discard;
    vec3 tangent = normalize(dFdx(position));
    vec3 binormal = normalize(dFdy(position));
    // vec3 normal = normalize(cross(tangent, binormal));

    const vec2 noiseScale = vec2(textureSize(noiseTexture, 0).xy);
    vec2 texRes = vec2(textureSize(positionTexture, 0).xy);
    vec4 random = texture(noiseTexture, texCoord.xy * (texRes / noiseScale));

    vec3 viewDir = camPos - position;
    float viewDistance = length(viewDir);
    viewDir = viewDir / viewDistance;
    float rayRadius = 0.5 * aoRadius * texRes.x / viewDistance;

    float tTcos = acos(dot(normal, viewDir));
    float tTtan = atan(tangent.z / length(tangent.xy));

    // ao = inverse(view) * vec4(tangent, 1);
    // ao = vec4(tTcos, tTtan, 0, 1.0);
    // return;

    float aoVal = 1.0;
    if (rayRadius > 1.0) {
        aoVal = 0.0f;
        
        float numSteps;
        vec2 stepSizeUV;

        ComputeSteps(stepSizeUV, numSteps, rayRadius, random.z);
        float alpha = 2.0 * PI / numDirections;

        for(float d = 0; d < numDirections; ++d) {
            float theta = alpha * d;

            vec2 dir = vec2(cos(theta), sin(theta));

            // dir = vec2(dir.x * random.x - dir.y * random.y, dir.x * random.y + dir.y * random.x);
            vec2 deltaUV = dir * stepSizeUV;

            // aoVal += HorizonOcclusion(deltaUV, position, tangent, binormal, random.z, numSteps);
            aoVal += HBAO(deltaUV, position, normal, random.z, numSteps) / numSteps;
            // aoVal += HBAO2(deltaUV, position, viewDir, normal, tangent, random.w, numSteps);//  / (2.0 * PI);
        }
        aoVal = max(1.0 - (aoVal / (numDirections)) * AOStrength, 0.0);
        // aoVal = max(1.0 - (aoVal / (numDirections)), 0.0);
    }

    if (aoVal <= 0.0031308) aoVal = 12.92 * aoVal;
    else aoVal = 1.055 * pow(aoVal, (1.0/2.4)) - 0.055;

    ao = vec4(aoVal, aoVal, aoVal, 1.0);
    // ao = vec4(aoVal * texture(directIlluminationTexture, texCoord).rgb, 1.0);
}
