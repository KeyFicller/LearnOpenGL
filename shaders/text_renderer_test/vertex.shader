#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;

uniform float z_offset;

void main()
{
    gl_Position = vec4(vertex.xy, z_offset, 1.0);
    TexCoords = vertex.zw;
}