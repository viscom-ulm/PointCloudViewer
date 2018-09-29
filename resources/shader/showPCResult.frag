#version 330 core

uniform vec3 camPos;

in GS_OUT {
    vec3 position;
    vec3 normal;
    vec3 result;
    vec2 localCoords;
} ps_in;

out vec4 color;

void main()
{
    vec2 p2 = ps_in.localCoords;
    float p = dot(p2, p2);
    if (p > 1.0) discard;

    // if (dot(ps_in.normal, camPos - ps_in.position) < 0) discard;

    // if (p > 0.8) {
    //     color = vec4(0, 0, 0, 1);
    //     return;
    // }
    const float sigma = 0.8;
    const float pi = 3.1415926535897932384626433832795;

    float gexp = p / (sigma * sigma);
    //float w = /*(1.0 / (sigma * sqrt(2 * pi))) */ exp(-0.5 * gexp);
    float w = 1.0;

    color = vec4(w * ps_in.result, w);
}
