#version 330 core

in vec3 vPos;
in vec2 vUv;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

// b is "close to a" -> 1, with a soft edge of `edge` units/pixels
float line(float a, float b, float width, float edge)
{
    float half_width = 0.5 * width;
    return smoothstep(a - half_width - edge, a - half_width, b) -
           smoothstep(a + half_width, a + half_width + edge, b);
}

void main()
{
    float t = u_time * 0.6;

    // red  : y = k*x in clip space, slope k driven by the cursor height
    float k = mix(-1.2, 1.2, u_mouse.y);
    float red = line(k * vPos.x, vPos.y, 0.03, 0.001);

    // green: the same kind of test but measured in gl_FragCoord pixels.
    // the offset is derived from u_resolution so it passes through the centre.
    vec2 px = gl_FragCoord.xy;
    float off = u_resolution.x - 0.5 * u_resolution.y;
    float green = line(2.0 * px.x, px.y + off, 14.0, 2.0);

    // blue : y = sin(2*pi*x + t) in clip space, animated phase
    float wave = sin(vPos.x * 3.1415926 * 2.0 + t) * 0.65;
    float blue = line(vPos.y, wave, 0.04, 0.002);

    // white: y = 0.5*x in uv space (0..1), independent of the clip range
    float white = line(0.5 * vUv.x, vUv.y, 0.025, 0.001);

    // wide, dim copies give every line a glow
    float glow = line(k * vPos.x, vPos.y, 0.10, 0.05)
               + line(vPos.y, wave, 0.14, 0.06)
               + line(0.5 * vUv.x, vUv.y, 0.09, 0.04);

    vec3 col = vec3(0.06, 0.07, 0.11) + vec3(0.10, 0.12, 0.18) * glow;

    col += vec3(1.00, 0.30, 0.30) * red;
    col += vec3(0.30, 1.00, 0.45) * green;
    col += vec3(0.35, 0.60, 1.00) * blue;
    col += vec3(0.95, 0.95, 0.95) * white;

    // faint axes so the coordinate systems stay readable
    col += vec3(0.22) * line(0.0, vPos.y, 0.006, 0.004);
    col += vec3(0.22) * line(0.0, vPos.x, 0.006, 0.004);

    FragColor = vec4(col, 1.0);
}
