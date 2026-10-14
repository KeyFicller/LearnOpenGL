#version 330 core

out vec4 FragColor;
uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

// The trick: push the sample point outward by a travelling wave, then take
// 1/distance to the nearest point of a wrapping grid. The three passes use
// slightly different phases, which tints the hot spots.
void main()
{
    float aspect = u_resolution.x / u_resolution.y;

    // square, centred coordinates (pixel based, so the edges stay sharp)
    vec2 q = gl_FragCoord.xy / u_resolution - 0.5;
    q.x *= aspect;

    // cursor drags the centre of the light well
    vec2 m = (u_mouse - 0.5) * vec2(aspect, 1.0) * 0.7;

    vec2 d0 = q - m;
    float l = max(length(d0), 0.03);
    vec2 dir = d0 / l;

    vec3 c;
    float z = u_time;

    for (int i = 0; i < 3; i++)
    {
        float fi = float(i);

        z += 0.07;
        // standing wave travelling outward from the well
        float wobble = (sin(z + fi * 0.6) + 1.0)
                     * abs(sin(l * 9.0 - z - z + fi * 0.5));
        vec2 wuv = q + dir * wobble * 0.35;

        // wrapping the coordinates produces the repeated bright points
        float d = length(mod(wuv, 1.0) - 0.5);
        c[i] = 0.012 / max(d, 0.004);
    }

    c /= l;

    // tone map so the cores stay white instead of clipping to a flat blob
    vec3 col = vec3(1.0) - exp(-c * 1.7);
    col = pow(col, vec3(0.4545));
    col *= vec3(0.92, 0.96, 1.00);

    // gentle vignette
    col *= 1.0 - 0.35 * smoothstep(0.5, 1.4, length(q));

    FragColor = vec4(col, 1.0);
}
