#version 330 core

in vec3 vPos;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

// hard edge: step only
float circle(vec2 pt, vec2 center, float radius)
{
    return 1.0 - step(radius, length(pt - center));
}

// soft edge: smoothstep across a band
float circle(vec2 pt, vec2 center, float radius, int soften)
{
    float edge = (soften > 0) ? 0.1 * radius : 0.0;
    return 1.0 - smoothstep(radius - edge, radius + edge, length(pt - center));
}

// outline: difference of two step tests
float circle(vec2 pt, vec2 center, float radius, float width)
{
    if (width > radius)
        return 0.0;
    return step(radius - width, length(pt - center)) - step(radius + width, length(pt - center));
}

void main()
{
    vec2 uv = vPos.xy;
    uv.x *= u_resolution.x / u_resolution.y;   // keep circles round

    float t = u_time * 0.7;
    float r = 0.30 + 0.03 * sin(t);            // all three share one animated radius
    float w = 0.02 + 0.02 * (0.5 + 0.5 * sin(t)); // outline thickness breathes too

    vec2 cA = vec2(-1.00, 0.0);   // hard
    vec2 cB = vec2( 0.00, 0.0);   // soft
    vec2 cC = vec2( 1.00, 0.0);   // outline

    float A = circle(uv, cA, r);
    float B = circle(uv, cB, r, 1);
    float C = circle(uv, cC, r, w);

    // every shape also gets a faint guide ring so the shared radius is readable
    float guide = circle(uv, cA, r + 0.14, 0.006)
                + circle(uv, cB, r + 0.14, 0.006)
                + circle(uv, cC, r + 0.14, 0.006);

    vec3 col = vec3(0.05, 0.055, 0.09);
    col += vec3(0.10, 0.10, 0.14) * guide;

    col = mix(col, vec3(0.95, 0.78, 0.22), A);   // yellow  : step
    col = mix(col, vec3(0.15, 0.85, 0.90), B);   // cyan    : smoothstep
    col = mix(col, vec3(0.95, 0.35, 0.75), C);   // magenta : ring

    // faint grid so the space is readable
    vec2 g = abs(fract(uv * 4.0) - 0.5);
    col *= 1.0 - 0.12 * (1.0 - step(0.02, min(g.x, g.y)));

    // cursor glow to show the soft circle responds smoothly
    vec2 m = vec2((u_mouse.x - 0.5) * u_resolution.x / u_resolution.y, u_mouse.y - 0.5);
    col += vec3(0.25, 0.30, 0.40) / (1.0 + 25.0 * dot(uv - m, uv - m));

    FragColor = vec4(col, 1.0);
}
