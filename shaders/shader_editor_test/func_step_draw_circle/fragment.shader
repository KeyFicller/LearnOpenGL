#version 330 core

in vec3 vPos;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

void main()
{
    // aspect-correct so circles stay round
    vec2 uv = vPos.xy;
    uv.x *= u_resolution.x / u_resolution.y;

    float d = length(uv);
    float a = atan(uv.y, uv.x);

    // cursor drags the main radius around a visible base size
    vec2 m = vec2((u_mouse.x - 0.5) * u_resolution.x / u_resolution.y, u_mouse.y - 0.5);
    float mr = clamp(0.34 + length(m), 0.18, 1.15);

    vec3 col = vec3(0.05, 0.06, 0.10);

    // step(edge, x) = x < edge ? 0 : 1  ->  filling the disc
    float disc = 1.0 - step(mr, d);
    col = mix(col, vec3(0.16, 0.55, 0.98), disc);

    // hard-ring band = product of two step tests
    float band = (1.0 - step(mr + 0.20, d)) * step(mr + 0.11, d);
    col = mix(col, vec3(0.98, 0.78, 0.28), band);

    // a slowly breathing ring at a fixed radius
    float r1 = 0.62 + 0.10 * sin(u_time * 0.9);
    float ring = (1.0 - step(r1 + 0.03, d)) * step(r1, d);
    col = mix(col, vec3(0.95, 0.34, 0.58), ring);

    // dashed outer ring: step on the angle, not the radius
    float dash  = step(0.5, fract(a / 6.28318 * 24.0));
    float outer = (1.0 - step(1.06, d)) * step(0.97, d) * dash;
    col = mix(col, vec3(0.85, 0.95, 1.0), outer);

    // faint concentric guide curves so the distance field itself is visible
    col += 0.05 * (1.0 - step(0.014, abs(fract(d * 8.0) - 0.5)));

    FragColor = vec4(col, 1.0);
}
