#version 450 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
    vec3 position;
    vec3 normal;
    vec3 result;
} gs_in[];

uniform mat4 MVP;
uniform float bbRadius;

out GS_OUT {
    vec3 position;
    vec3 normal;
    vec3 result;
    vec2 localCoords;
} gs_out;

void main()
{
    vec3 b = normalize(vec3(0, -1, -1));
    if (gs_in[0].normal.x <= gs_in[0].normal.y && gs_in[0].normal.x <= gs_in[0].normal.z) b = vec3(1, 0, 0);
    else if (gs_in[0].normal.y <= gs_in[0].normal.z) b = vec3(0, 1, 0);
    else b = vec3(0, 0, 1);

    vec3 tangent = normalize(cross(gs_in[0].normal, b));
    vec3 binormal = normalize(cross(gs_in[0].normal, tangent));

    float hlfPtSize = (bbRadius * .15f) / gl_in[0].gl_Position.w;
    gs_out.normal = gs_in[0].normal;
    gs_out.result = gs_in[0].result;
    gs_out.position = gs_in[0].position - hlfPtSize * tangent + hlfPtSize * binormal;
    gs_out.localCoords = vec2(-1,1);
    gl_Position = MVP * vec4(gs_out.position, 1.0);
    EmitVertex();
    gs_out.position = gs_in[0].position - hlfPtSize * tangent - hlfPtSize * binormal;
    gs_out.localCoords = vec2(-1,-1);
    gl_Position = MVP * vec4(gs_out.position, 1.0);
    EmitVertex();
    gs_out.position = gs_in[0].position + hlfPtSize * tangent + hlfPtSize * binormal;
    gs_out.localCoords = vec2(1,1);
    gl_Position = MVP * vec4(gs_out.position, 1.0);
    EmitVertex();
    gs_out.position = gs_in[0].position + hlfPtSize * tangent - hlfPtSize * binormal;
    gs_out.localCoords = vec2(1,-1);
    gl_Position = MVP * vec4(gs_out.position, 1.0);
    EmitVertex();
    EndPrimitive();
}
