#version 330 core
layout(location = 0) in vec3 aPos;

out vec3 vs_position;

void main()
{
    vs_position = aPos;
    gl_Position = vec4(aPos, 1.0);
}