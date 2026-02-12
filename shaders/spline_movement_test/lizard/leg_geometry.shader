#version 330 core
layout (lines) in;
layout (triangle_strip, max_vertices = 60) out;

in float vs_progress[];
uniform float uLegSize;
uniform float uOffsetRatio;

void main() {
    vec3 dir = normalize(gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz);
    vec3 perp = vec3(-dir.y, dir.x, 0.0);

    int segments = 10;
        // 1.0  0.8  0.6
        float size0 = uLegSize * (1.0f - vs_progress[0] * 0.4);
        float size1 = uLegSize * (1.0f - vs_progress[1] * 0.4);
    for (int i = 0; i <= segments; i++) {
        float t = float(i) / float(segments);
        vec3 pos = mix(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, t);


        float radius = mix(size0, size1, t) * uOffsetRatio;

        // Strip out put
        gl_Position = vec4(pos + perp * radius, 1.0);
        EmitVertex();

        gl_Position = vec4(pos - perp * radius, 1.0);
        EmitVertex();
    }

    EndPrimitive();

    if (0.49f < vs_progress[0] && vs_progress[0] < 0.51f) {
            float pi = 3.1415926535897932384626433832795;
    
    int segments = 16;
        // Generate circle outline for body segments
        for (int i = 0; i <= segments; i++) {
            float segmentRadius = uLegSize * uOffsetRatio;

            float angle = (float(i) / float(segments)) * 2.0 * pi;
            vec3 offset = vec3(cos(angle) * segmentRadius, sin(angle) * segmentRadius, 0.0);
            // inner circle
            gl_Position = vec4(gl_in[0].gl_Position.xyz, 1.0);
            EmitVertex();
            // outer circle
            gl_Position = vec4(gl_in[0].gl_Position.xyz + offset * 1.1f, 1.0);
            EmitVertex();
        }
    }

}