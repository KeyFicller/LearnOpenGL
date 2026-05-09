#version 330 core

in vec3 vPos;
in vec2 vUv;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

float random(vec2 pt, float seed)
{
    const float a = 12.9898;
    const float b = 78.223;
    const float c = 43758.543123;
    return fract(sin(dot(pt, vec2(a, b)) + seed) * c);
}

void main()
{
   vec3 color = vec3(1.0) * random(vUv, u_time);
    FragColor = vec4(color, 1.0);
}
