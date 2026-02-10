#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 60) out;

in vec3 vs_position[];
in float vs_progress[];
uniform vec3 uShapeFactor;
uniform float uBaseRadius;

void main() {
    float pi = 3.1415926535897932384626433832795;
    
    int segments = 16;
        // Generate circle outline for body segments
        for (int i = 0; i <= segments; i++) {
            float ratio = uShapeFactor.x * vs_progress[0] * vs_progress[0] + uShapeFactor.y * vs_progress[0] + uShapeFactor.z;
            float segmentRadius = uBaseRadius * ratio;

            float angle = (float(i) / float(segments)) * 2.0 * pi;
            vec3 offset = vec3(cos(angle) * segmentRadius, sin(angle) * segmentRadius, 0.0);
            // inner circle
            gl_Position = vec4(vs_position[0] + offset * 0.9f, 1.0);
            EmitVertex();
            // outer circle
            gl_Position = vec4(vs_position[0] + offset * 1.1f, 1.0);
            EmitVertex();
        }
}