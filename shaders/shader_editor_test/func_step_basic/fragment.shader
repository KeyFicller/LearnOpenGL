#version 330 core

in vec3 vPos;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

void main()
{
    vec3 color;
    // step(x, y) eg. y < x ? 0.0 : 1.0
    color.r = step(0.0, vPos.x);
    color.g = step(0.0, vPos.y);
    FragColor = vec4(color, 1.0);
}
