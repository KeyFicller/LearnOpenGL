#version 330 core

in vec3 vPos;
in vec2 vUv;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

float noise(vec2 pt, float seed)
{
    const float a = 1.9898;
    const float b = 7.223;
    const float c = 43.543123;
    return fract(sin(dot(pt, vec2(a, b)) + seed) * c);
}

void main()
{
    // Scale coordiante system to see.
    vec2 pos = vec2(vUv * 10.0);
    // Some noise in action
    pos.y -= u_time;
    
    // Use noise function
    float n = noise(pos, 0.0);
    
    
   vec3 color = vec3(n);
    FragColor = vec4(color, 1.0);
}
