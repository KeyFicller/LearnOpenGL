#version 330 core

in vec3 vPos;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

float rect(vec2 pt, vec2 anchor, vec2 center, vec2 size)
{
    vec2 v = pt - center;
    vec2 half_size = size * 0.5;
    
    float horz = step(-half_size.x - anchor.x, v.x) - step(half_size.x - anchor.x, v.x);
    float vert = step(-half_size.y - anchor.y, v.y) - step(half_size.y - anchor.y, v.y);
    
    return horz * vert;
}

mat2 rotate_matrix(float theta)
{
    float c = cos(theta);
    float s = sin(theta);
    return mat2(c, -s, s, c);
}

void main()
{
    vec2 center = vec2(0.3, 0.0);
    vec2 pt = rotate_matrix(u_time) * (vPos.xy - center) + center;
    float in_rect = rect(pt, vec2(-0.2,0.2), center, vec2(0.4));
    FragColor = vec4(vec3(0.8, 0.6, 0.0) * in_rect, 1.0);
}
