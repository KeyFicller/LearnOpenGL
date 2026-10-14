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

mat2 rot(float a)
{
    float c = cos(a), s = sin(a);
    return mat2(c, -s, s, c);
}

void main()
{
    float aspect = u_resolution.x / u_resolution.y;

    vec2 uv = vPos.xy;
    uv.x *= aspect;

    float t = u_time * 0.9;

    // orbit radius; the cursor pushes the path outward
    vec2 m = (u_mouse * 2.0 - 1.0) * vec2(aspect, 1.0);
    float r = 0.75 + 0.12 * sin(t * 0.7) + 0.20 * length(m);

    vec2 center = r * vec2(cos(t), sin(t));

    vec3 col = vec3(0.06, 0.07, 0.11);

    // dotted orbit path so the motion is readable as a track
    float a = atan(uv.y, uv.x);
    float d = abs(length(uv) - r);
    float dots = step(0.5, fract(a / 6.28318 * 44.0));
    col += vec3(0.34, 0.38, 0.48) * (1.0 - step(0.007, d)) * dots;

    // ghost trail: the same shape sampled a few frames in the past
    for (int i = 0; i < 6; i++)
    {
        float k = float(i);
        float tt = t - 0.09 * (k + 1.0);
        vec2  c  = r * vec2(cos(tt), sin(tt));
        vec2  p  = rot(-tt) * (uv - c);
        float inGhost = rect(p, vec2(0.0), vec2(0.50));
        col = mix(col, vec3(0.95, 0.45, 0.15), inGhost * (0.40 / (k + 1.0)));
    }

    // the shape itself: translate by center, rotate by t
    vec2 p = rot(-t) * (uv - center);
    float inShape = rect(p, vec2(0.0), vec2(0.52));
    float outline = rect(p, vec2(0.0), vec2(0.60)) - inShape;

    col = mix(col, vec3(0.98, 0.76, 0.20), inShape);
    col = mix(col, vec3(1.00, 0.96, 0.82), outline);

    FragColor = vec4(col, 1.0);
}
