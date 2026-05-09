#version 330 core

layout(location = 0) in vec3 aPos;
out vec3 vPos;
out vec2 vUv;

void main()
{
    vPos = aPos;
    vUv = (vPos.xy + vec2(1.0)) * 0.5;
    gl_Position = vec4(aPos, 1.0);
}
