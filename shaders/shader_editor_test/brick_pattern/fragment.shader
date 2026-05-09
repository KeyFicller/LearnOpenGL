#version 330 core

in vec3 vPos;
in vec2 vUv;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

float line(float a, float b, float width, float edge)
{
    float half_width = 0.5 * width;
    return smoothstep(a - half_width - edge, a - half_width, b) - 
        smoothstep(a + half_width, a + half_width + edge, b);
}

float brick(vec2 pt, float mortar, float edge)
{
    float result = line(pt.y, 0, mortar, edge);
    result += line(pt.y, 0.5, mortar, edge);
    result += line(pt.y, 1.0, mortar, edge);
    
    if (pt.y > 0.5)
        pt.x = fract(pt.x + 0.5);
   result += line(pt.x, 0.5, mortar, edge);
    
    return result;
}

void main()
{
   float tile_count = 10.0;
   vec2 uv = fract(vUv * tile_count);
   vec3 brick_color = vec3(0.8, 0.3, 0.3);
   vec3 mortar_color = vec3(1.0);
   vec3 color = mix(brick_color, mortar_color, brick(uv, 0.05, 0.01));
    FragColor = vec4(color, 1.0);
}
