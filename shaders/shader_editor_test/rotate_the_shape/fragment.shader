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

mat2 rotate_matrix(float theta)
{
    float c = cos(theta);
    float s = sin(theta);
    return mat2(c, -s, s, c);
}

float line(float a, float b, float width, float edge)
{
    float h = 0.5 * width;
    return smoothstep(a - h - edge, a - h, b) - smoothstep(a + h, a + h + edge, b);
}

void main()
{
    float aspect = u_resolution.x / u_resolution.y;

    vec2 uv = vPos.xy;
    uv.x *= aspect;

    // the cursor sets speed and direction; angle = time * speed
    float speed = mix(-2.5, 2.5, u_mouse.x);
    float theta = u_time * speed;

    vec2 pt = rotate_matrix(theta) * uv;

    vec3 col = vec3(0.06, 0.07, 0.11);

    // static reference outline: it stays put while the shape spins
    float ref = rect(uv, vec2(0.0), vec2(0.58)) - rect(uv, vec2(0.0), vec2(0.50));
    col = mix(col, vec3(0.24, 0.30, 0.42), ref);

    // the rotating square, drawn in the rotated frame
    float in_rect = rect(pt, vec2(0.0), vec2(0.50));
    float edge    = rect(pt, vec2(0.0), vec2(0.58)) - in_rect;
    col = mix(col, vec3(0.82, 0.58, 0.12), in_rect);
    col = mix(col, vec3(1.00, 0.92, 0.60), edge);

    // a spoke along the rotated +x axis, so the angle is readable
    float spoke = line(0.0, pt.y, 0.012, 0.004) * step(0.0, pt.x) * step(length(pt), 1.0);
    col += vec3(0.20, 0.85, 0.95) * spoke;

    // a dot riding the same rotation at a larger radius
    vec2 dotPos = rotate_matrix(theta) * vec2(0.78, 0.0);
    col += vec3(1.0, 0.45, 0.35) * smoothstep(0.05, 0.0, length(uv - dotPos));

    // trail of the dot over the last half turn
    float a = mod(theta, 6.28318) - atan(uv.y, uv.x);
    float trail = exp(-mod(a + 6.28318, 6.28318) * 1.5) * (1.0 - smoothstep(0.0, 0.06, abs(length(uv) - 0.78)));
    col += vec3(0.9, 0.35, 0.25) * trail * 0.6;

    FragColor = vec4(col, 1.0);
}
