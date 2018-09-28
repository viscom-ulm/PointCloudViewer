#version 330 core

// Ouput data
layout(location = 0) out vec4 fragmentdepth;

void main()
{
    // Not really needed, OpenGL does it anyway
    fragmentdepth = vec4(vec3(gl_FragCoord.z), 1);
}
