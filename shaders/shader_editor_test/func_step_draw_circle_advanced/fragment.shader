#version 330 core

in vec3 vPos;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

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

void main()
{
    float in_solid_circle = circle(vPos.xy, vec2(-0.5, 0.5), 0.2);
    float in_soften_circle = circle(vPos.xy, vec2(0.5, 0.5), 0.2, 1);
    float in_circle_ring = circle(vPos.xy, vec2(0.5, -0.5), 0.23, 0.03);
    FragColor = vec4(vec3(0.6, 0.6, 0.0) * in_solid_circle
     + vec3(0.0, 0.6, 0.6) * in_soften_circle + vec3(0.6, 0.0, 0.6) * in_circle_ring , 1.0);
}
