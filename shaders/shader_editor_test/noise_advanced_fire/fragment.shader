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

// smooth value noise built on top of the hash above
float vnoise(vec2 p, float seed)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    float a = noise(i, seed);
    float b = noise(i + vec2(1.0, 0.0), seed);
    float c = noise(i + vec2(0.0, 1.0), seed);
    float d = noise(i + vec2(1.0, 1.0), seed);

    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

// layered octaves give the flame its ragged detail
float fbm(vec2 p, float seed)
{
    float v = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 5; i++)
    {
        v += amp * vnoise(p, seed);
        p *= 2.03;
        p = mat2(0.8, -0.6, 0.6, 0.8) * p;
        amp *= 0.5;
    }
    return v;
}

void main()
{
    vec2 uv = vUv;
    float t = u_time;

    // the flame scrolls upward: subtracting time from y does that
    vec2 p = vec2(uv.x * 3.0, uv.y * 2.2 - t * 1.5);
    float n = fbm(p, 0.0);

    // the cursor is a small heat source, the bottom of the screen is the hot bed
    vec2 m = u_mouse;
    float d = length(vec2((uv.x - m.x) * 1.6, (uv.y - m.y) * 2.2));
    float heat = clamp(1.15 - d * 1.3, 0.0, 1.0) * 0.9 + pow(1.0 - uv.y, 1.6) * 0.85;

    float f = clamp(heat + n * 0.85 - 0.55, 0.0, 1.0);

    // black -> red -> orange -> yellow -> white ramp
    vec3 col = mix(vec3(0.02, 0.00, 0.01), vec3(0.55, 0.04, 0.00), smoothstep(0.00, 0.28, f));
    col = mix(col, vec3(1.00, 0.38, 0.03), smoothstep(0.28, 0.60, f));
    col = mix(col, vec3(1.00, 0.80, 0.28), smoothstep(0.60, 0.84, f));
    col = mix(col, vec3(1.00, 0.98, 0.88), smoothstep(0.84, 1.00, f));

    // embers: sparse hashed sparks that rise and fade
    vec2 sp = vec2(uv.x * 26.0, uv.y * 14.0 - t * 3.0);
    vec2 id = floor(sp);
    vec2 gv = fract(sp) - 0.5;
    float h = noise(id, 7.0);
    vec2 off = (vec2(noise(id, 3.1), noise(id, 5.7)) - 0.5) * 0.6;
    float spark = smoothstep(0.07, 0.0, length(gv - off)) * step(0.85, h);
    col += vec3(1.0, 0.65, 0.25) * spark * smoothstep(0.0, 0.5, uv.y);

    FragColor = vec4(col, 1.0);
}
