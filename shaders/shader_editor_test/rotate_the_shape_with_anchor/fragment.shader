#version 330 core

in vec3 vPos;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

// anchor is the point of the shape that sits at "center"
float rect(vec2 pt, vec2 anchor, vec2 center, vec2 size)
{
    vec2 v = pt - center;
    vec2 half_size = size * 0.5;

    float horz = step(-half_size.x - anchor.x, v.x) - step(half_size.x - anchor.x, v.x);
    float vert = step(-half_size.y - anchor.y, v.y) - step(half_size.y - anchor.y, v.y);

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

float ring(vec2 pt, vec2 c, float radius, float width)
{
    return 1.0 - step(width, abs(length(pt - c) - radius));
}

void main()
{
    float aspect = u_resolution.x / u_resolution.y;

    vec2 uv = vPos.xy;
    uv.x *= aspect;

    // the cursor moves the pivot
    vec2 pivot = (u_mouse - 0.5) * vec2(aspect, 1.0) * 1.4;
    float theta = u_time * 0.8;
    float R = 0.55;

    vec3 col = vec3(0.06, 0.07, 0.11);

    // dotted path the shape centre travels along
    float a = atan(uv.y - pivot.y, uv.x - pivot.x);
    float d = abs(length(uv - pivot) - R);
    col += vec3(0.25, 0.30, 0.42) * (1.0 - step(0.006, d)) * step(0.5, fract(a / 6.28318 * 40.0));

    // shape A: the square is placed at pivot + (R, 0), then the *whole plane*
    // is rotated about the pivot, so the square orbits instead of spinning
    vec2 centerA = pivot + vec2(R, 0.0);
    vec2 ptA = rotate_matrix(theta) * (uv - pivot) + pivot;

    float inA   = rect(ptA, vec2(0.0), centerA, vec2(0.34));
    float edgeA = rect(ptA, vec2(0.0), centerA, vec2(0.40)) - inA;
    col = mix(col, vec3(0.85, 0.60, 0.12), inA);
    col = mix(col, vec3(1.00, 0.93, 0.65), edgeA);

    // the radius arm from the pivot to the shape, drawn in the rotated frame
    float arm = line(pivot.y, ptA.y, 0.008, 0.003)
              * step(pivot.x, ptA.x)
              * step(length(ptA - pivot), R);
    col += vec3(0.20, 0.85, 0.95) * arm;

    // shape B, for comparison: the same square rotating about its own centre
    vec2 centerB = pivot + vec2(0.0, -0.85);
    vec2 ptB = rotate_matrix(theta) * (uv - centerB) + centerB;
    float inB = rect(ptB, vec2(0.0), centerB, vec2(0.34));
    col = mix(col, vec3(0.22, 0.42, 0.85), inB * 0.9);

    // the pivot itself
    col += vec3(1.0, 0.45, 0.35) * (1.0 - step(0.020, length(uv - pivot)));
    col += vec3(1.0, 0.45, 0.35) * ring(uv, pivot, 0.10, 0.005);

    FragColor = vec4(col, 1.0);
}
