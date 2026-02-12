#version 330 core
layout(location = 0) in vec3 aPos;

out float vs_progress;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    vs_progress = gl_VertexID / 2.0f;
}

