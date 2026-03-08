#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 60) out;

void main() {
    float pi = 3.1415926535897932384626433832795;
    vec2 position = gl_in[0].gl_Position.xy;
    int segments = 16;
        // Generate circle outline for body segments
        for (int i = 0; i <= segments; i++) {
            float segmentRadius = 0.02f;

            float angle = (float(i) / float(segments)) * 2.0 * pi;
            vec2 offset = vec2(cos(angle) * segmentRadius, sin(angle) * segmentRadius);
            // inner circle
            gl_Position = vec4(position + offset * 0.9f, 0.0, 1.0);
            EmitVertex();
            // outer circle
            gl_Position = vec4(position + offset * 1.1f, 0.0, 1.0);
            EmitVertex();
        }
}