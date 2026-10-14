#version 330 core

in vec3 vPos;
in vec2 vUv;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

float random(vec2 pt, float seed)
{
    const float a = 12.9898;
    const float b = 78.223;
    const float c = 43758.543123;
    return fract(sin(dot(pt, vec2(a, b)) + seed) * c);
}

void main()
{
    // 2x2 grid of panes, all built from the same hash
    vec2 luv = fract(vUv * 2.0);      // 0..1 inside a pane
    vec2 lc  = luv - 0.5;             // -0.5..0.5

    float right  = step(0.5, vUv.x);
    float bottom = 1.0 - step(0.5, vUv.y);
    vec2  m      = (u_mouse - 0.5) * 2.0;

    // bottom-left: animated static, quantised to 4 levels
    float A = floor(random(luv, floor(u_time * 20.0)) * 4.0) / 3.0;

    // bottom-right: the same noise held longer and posterised to 8 levels
    float B = floor(random(luv + m * 0.1, floor(u_time * 2.0)) * 8.0) / 7.0;

    // top-left: hash as a dither threshold against a linear ramp
    float ramp  = luv.x;
    float thr   = random(floor(luv * 160.0), 0.0);
    float C     = step(thr, ramp);

    // top-right: one hashed star per cell
    vec2  sc  = floor(lc * 26.0);
    vec2  off = vec2(random(sc, 1.0), random(sc, 7.0)) - 0.5;
    float d   = length(fract(lc * 26.0) - 0.5 - off);
    float h   = random(sc, 3.0);
    float D   = smoothstep(0.20, 0.0, d) * step(0.72, h);

    vec3 colA = vec3(0.90) * A;
    vec3 colB = vec3(0.20, 0.95, 0.62) * B;
    vec3 colC = vec3(0.20, 0.55, 1.00) * C;
    vec3 colD = vec3(0.09, 0.10, 0.16) + vec3(1.00, 0.90, 0.70) * D;

    vec3 col = mix(colA, colB, right);
    col = mix(col, mix(colC, colD, right), bottom);

    // thin gutters between the panes
    vec2 g = min(fract(vUv * 2.0), 1.0 - fract(vUv * 2.0));
    col *= 1.0 - 0.85 * (1.0 - step(0.006, min(g.x, g.y)));

    FragColor = vec4(col, 1.0);
}
