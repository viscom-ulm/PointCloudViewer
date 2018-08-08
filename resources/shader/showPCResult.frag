#version 330 core

uniform vec3 camPos;

in vec3 vertPosition;
in vec3 vertNormal;
in vec3 vertResult;

out vec4 color;

void main()
{
    vec2 p2 = gl_PointCoord.xy * 2.0 - 1.0;
    float p = dot(p2, p2);
    if (p > 1.0) discard;

    if (dot(vertNormal, camPos - vertPosition) < 0) discard;

    // if (p > 0.8) {
    //     color = vec4(0, 0, 0, 1);
    //     return;
    // }
    const float sigma = 0.8;
    const float pi = 3.1415926535897932384626433832795;

    float gexp = p / (sigma * sigma);
    //float w = /*(1.0 / (sigma * sqrt(2 * pi))) */ exp(-0.5 * gexp);
    float w = 1.0;

    color = vec4(w * vertResult, w);
}
