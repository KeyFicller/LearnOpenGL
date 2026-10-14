#version 330 core

in vec3 vPos;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

// two perpendicular step() bands multiplied together
float rect(vec2 pt, vec2 center, vec2 size)
{
    vec2 v = pt - center;
    vec2 half_size = size * 0.5;

    float horz = step(-half_size.x, v.x) - step(half_size.x, v.x);
    float vert = step(-half_size.y, v.y) - step(half_size.y, v.y);

    return horz * vert;
}

// an outline is just "big rect minus small rect"
float rectOutline(vec2 pt, vec2 center, vec2 size, float w)
{
    return rect(pt, center, size + 2.0 * w) - rect(pt, center, size);
}

void main()
{
    float aspect = u_resolution.x / u_resolution.y;

    vec2 uv = vPos.xy;
    uv.x *= aspect;

    vec2 m = (u_mouse * 2.0 - 1.0) * vec2(aspect, 1.0);

    vec3 col = vec3(0.05, 0.06, 0.09);

    // faint checkerboard to show the cells the rectangles sit in
    vec2 cell = floor(uv * 4.0);
    col += vec3(0.025, 0.030, 0.045) * mod(cell.x + cell.y, 2.0);

    // fixed rectangle, size breathes with time
    vec2 sizeA = vec2(0.90 + 0.18 * sin(u_time * 1.1), 0.60 + 0.12 * sin(u_time * 0.7));
    float inA    = rect(uv, vec2(0.0), sizeA);
    float edgeA  = rectOutline(uv, vec2(0.0), sizeA, 0.02);
    col = mix(col, vec3(0.16, 0.55, 0.95), inA * 0.85);
    col = mix(col, vec3(0.70, 0.92, 1.00), edgeA);

    // rectangle that follows the cursor
    float inB   = rect(uv, m, vec2(0.55, 0.35));
    float edgeB = rectOutline(uv, m, vec2(0.55, 0.35), 0.02);
    col = mix(col, vec3(0.95, 0.30, 0.30), inB * 0.85);
    col = mix(col, vec3(1.00, 0.80, 0.70), edgeB);

    // a column of small rectangles growing in and out, left edge
    vec2 sizeC = vec2(0.24 * (0.5 + 0.5 * sin(u_time * 1.6 + uv.y * 3.0)), 0.08);
    float inC = rect(uv, vec2(-aspect + 0.35, 0.0), sizeC);
    col = mix(col, vec3(0.30, 1.00, 0.55), inC);

    FragColor = vec4(col, 1.0);
}
