#version 330 core

in vec3 vPos;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

float rect(vec2 pt, vec2 center, vec2 size)
{
    vec2 v = pt - center;
    vec2 half_size = size * 0.5;
    
    float horz = step(-half_size.x, v.x) - step(half_size.x, v.x);
    float vert = step(-half_size.y, v.y) - step(half_size.y, v.y);
    
    return horz * vert;
}

void main()
{
    float path_radius = 0.3;
    vec2 center = vec2(path_radius * cos(u_time), path_radius * sin(u_time));
    float in_rect = rect(vPos.xy, center, vec2(0.3));
    FragColor = vec4(vec3(0.8, 0.6, 0.0) * in_rect, 1.0);
}
