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
    float h = clamp(dot(d, p) / dot(p, p), 0.0, 1.0);
    float l = length(d - p * h);

    float gradient = 0.0;
    const float gradient_angle = 3.1415926 * 1.2;

    if (length(d) < radius) {
        float angle = mod(theta + atan(d.y, d.x), 3.1415926 * 2);
        gradient = clamp(gradient_angle - angle, 0.0, gradient_angle) / gradient_angle * 0.5;
    }

    return gradient + 1.0 - smoothstep(width, width + edge, l);
}

float hash21(float n)
{
    return fract(sin(n * 127.1 + 311.7) * 43758.5453);
}

void main()
{
    float aspect = u_resolution.x / u_resolution.y;

    // aspect-corrected space so the radar stays round
    vec2 pt = vPos.xy;
    pt.x *= aspect;

    // the whole display drifts a little with the cursor
    vec2 center = (u_mouse - 0.5) * vec2(aspect, 1.0) * 0.6;
    vec2 d = pt - center;

    float theta = u_time * 2.0;
    float beamAng = mod(theta, 6.28318);
    float r = length(d);
    float ang = atan(d.y, d.x);

    vec3 axis_color  = vec3(0.25, 0.85, 0.45);
    vec3 sweep_color = vec3(0.35, 1.00, 0.55);
    vec3 color = vec3(0.02, 0.06, 0.03);

    // axis cross
    color += line(center.x, pt.x, 0.006, 0.002) * axis_color * 0.5;
    color += line(center.y, pt.y, 0.006, 0.002) * axis_color * 0.5;

    // range rings and radial spokes
    float rings = circle(d, vec2(0.0), 0.25, 0.004)
                + circle(d, vec2(0.0), 0.50, 0.004)
                + circle(d, vec2(0.0), 0.75, 0.004)
                + circle(d, vec2(0.0), 1.00, 0.004);
    color += rings * axis_color * 0.7;

    float spokes = 1.0 - step(0.015, abs(fract(ang / 6.28318 * 12.0) - 0.5));
    color += spokes * axis_color * 0.25 * step(r, 1.0);

    // rotating beam: a hard leading edge plus a fading wedge behind it
    float rel = mod(beamAng - ang + 6.28318, 6.28318);
    float lead = smoothstep(0.05, 0.0, min(rel, 6.28318 - rel));
    float wedge = exp(-rel * 1.2) * 0.5;
    color += (lead + wedge) * sweep_color * step(r, 1.0) * (1.0 - smoothstep(0.85, 1.0, r));
    color += sweep(pt, center, 1.0, 0.010, 0.004) * sweep_color * 0.8;

    // five contacts that light up as the beam sweeps past them
    for (int i = 0; i < 5; i++)
    {
        float fi = float(i);
        float a  = hash21(fi) * 6.28318;
        float rr = 0.25 + hash21(fi + 10.0) * 0.7;
        vec2  bp = rr * vec2(cos(a), sin(a));
        float dst = length(d - bp);
        float behind = mod(beamAng - a + 6.28318, 6.28318);
        float blip = smoothstep(0.03, 0.0, dst) * exp(-behind * 0.9);
        float halo = smoothstep(0.09, 0.0, dst) * exp(-behind * 2.0) * 0.25;
        color += (blip + halo) * vec3(0.75, 1.0, 0.8);
    }

    // faint scanlines
    color *= 1.0 - 0.10 * (0.5 + 0.5 * sin(pt.y * 320.0));

    FragColor = vec4(color, 1.0);
}
