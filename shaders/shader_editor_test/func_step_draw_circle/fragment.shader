#version 330 core

in vec3 vPos;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

void main()
{
    float in_circle = 1.0 - step(0.5, length(vPos.xy));
    FragColor = vec4(vec3(1.0, 0.0, 0.0) * in_circle, 1.0);
}
