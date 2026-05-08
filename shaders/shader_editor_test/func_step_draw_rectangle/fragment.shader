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
   vec2 uv = (u_mouse + vec2(-0.5)) * 2;
    float in_move_rect = rect(vPos.xy, uv, vec2(0.5));
    float in_fixed_rect = rect(vPos.xy, vec2(0.0), vec2(0.3));
    FragColor = vec4(vec3(1.0, 0.0, 0.0) * in_move_rect +
     vec3(0.0, 1.0, 0.0) * in_fixed_rect, 1.0);
}










