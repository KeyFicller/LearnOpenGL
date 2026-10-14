#version 330 core

in vec3 vPos;
in vec2 vUv;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

float hash21(vec2 p)
{
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

void main()
{
    float aspect = u_resolution.x / u_resolution.y;
    vec2 world = vec2(vUv.x * aspect, vUv.y);

    // running-bond layout: 8 rows, each cell twice as wide as it is tall
    vec2 p = world * vec2(4.0, 8.0);
    float row = floor(p.y);
    p.x += mod(row, 2.0) * 0.5;          // stagger every other row
    vec2 id = floor(p);
    vec2 f  = fract(p);

    float mortar = 0.055;
    vec2 d = min(f, 1.0 - f);
    float md = min(d.x, d.y);
    float isBrick = smoothstep(mortar, mortar + 0.015, md);

    // per-brick colour variation
    float rnd = hash21(id + row * 7.0);
    vec3 brick = mix(vec3(0.60, 0.18, 0.14), vec3(0.88, 0.42, 0.28), rnd);
    vec3 mortarCol = vec3(0.13, 0.12, 0.14);

    // bevel shading: top of each brick catches light, bottom falls into shade
    float bevel = clamp((f.y - 0.5) * 2.0, -1.0, 1.0);
    brick *= 0.85 + 0.35 * bevel;

    vec3 color = mix(mortarCol, brick, isBrick);

    // cursor works as a warm spotlight
    float spot = 1.0 / (1.0 + 40.0 * dot(vUv - u_mouse, vUv - u_mouse));
    color += brick * spot * 0.55;

    // slow soft sheen drifting upward
    float band = exp(-pow((fract(vUv.y * 1.0 - u_time * 0.07) - 0.5) * 6.0, 2.0));
    color += vec3(0.10, 0.07, 0.05) * band;

    FragColor = vec4(color, 1.0);
}
