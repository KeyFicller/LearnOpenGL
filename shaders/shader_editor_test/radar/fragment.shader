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

float circle(vec2 pt, vec2 center, float radius)
{
    return 1.0 - step(radius, length(pt - center));
}

float circle(vec2 pt, vec2 center, float radius, int soften)
{
    float edge = (soften > 0) ? 0.1 * radius : 0.0;
    return 1.0 - smoothstep(radius - edge, radius + edge, length(pt - center));
}

float circle(vec2 pt, vec2 center, float radius, float width)
{
    if (width > radius)
        return 0.0;
    return step(radius - width, length(pt - center)) - step(radius + width, length(pt - center));
}

float sweep(vec2 pt, vec2 center, float radius, float width, float edge)
{
    vec2 d = pt - center;
    float theta = u_time * 2.0;
    vec2 p = vec2(cos(theta), -sin(theta)) * radius;
    float h = clamp(dot(d, p)/ dot(p, p), 0.0, 1.0);
    float l = length(d - p * h);
    
    float gradient = 0.0;
    const float gradient_angle = 3.1415926 * 1.2;
    
    if (length(d) < radius) {
        float angle = mod(theta + atan(d.y, d.x), 3.1415926 * 2);
        gradient = clamp(gradient_angle - angle, 0.0, gradient_angle) / gradient_angle * 0.5;
    }
    
    return gradient + 1.0 - smoothstep(width, width + edge, l);
}

void main()
{
    vec3 color = vec3(0.0);
    vec3 axis_color = vec3(0.8);
    vec3 sweep_color = vec3(0.2, 0.2, 0.8);
    
    color += line(vPos.x, 0.0, 0.01, 0.001) * axis_color;
    color += line(0, vPos.y, 0.01, 0.001) * axis_color;
    color += circle(vPos.xy, vec2(0.0), 0.2, 0.005) * axis_color;
    color += circle(vPos.xy, vec2(0.0), 0.35, 0.005) * axis_color;
    color += circle(vPos.xy, vec2(0.0), 0.5, 0.005) * axis_color;
    color += sweep(vPos.xy, vec2(0.0), 0.5, 0.008, 0.001) * sweep_color;
    
    FragColor = vec4(color, 1.0);
}
