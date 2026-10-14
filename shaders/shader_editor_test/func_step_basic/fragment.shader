#version 330 core

in vec3 vPos;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

void main()
{
    // clip-space position, x and y both in [-1, 1]
    vec2 uv = vPos.xy;

    // two step thresholds: one follows the cursor, one drifts with time
    float edgeX = mix(-0.9, 0.9, u_mouse.x);
    float edgeY = sin(u_time * 0.8) * 0.8;

    // step(edge, x) = x < edge ? 0.0 : 1.0  -> a hard, binary edge
    float right = step(edgeX, uv.x);
    float top   = step(edgeY, uv.y);

    vec3 dark    = vec3(0.05, 0.06, 0.12);
    vec3 cyan    = vec3(0.10, 0.72, 0.88);
    vec3 magenta = vec3(0.90, 0.26, 0.62);
    vec3 amber   = vec3(1.00, 0.78, 0.26);

    // pick one of four flat colours from the two step results
    vec3 color = mix(dark, cyan, right);
    color = mix(color, magenta, top * (1.0 - right));
    color = mix(color, amber, top * right);

    // mark each step edge with a 1-pixel-ish bright line (still no smoothstep)
    float lineX = 1.0 - step(0.012, abs(uv.x - edgeX));
    float lineY = 1.0 - step(0.012, abs(uv.y - edgeY));
    color += vec3(0.85) * max(lineX, lineY);

    // faint grid so the flat regions still read as a surface
    vec2 g = abs(fract(uv * 5.0) - 0.5);
    color *= 1.0 - 0.10 * (1.0 - step(0.02, min(g.x, g.y)));

    FragColor = vec4(color, 1.0);
}
