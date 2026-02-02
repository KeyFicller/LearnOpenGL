#version 330 core
layout (points) in;
layout (line_strip, max_vertices = 30) out;

void main() {
    float radius = 0.02f;
    float Pi = 3.1415926535897932384626433832795;
    for (int i = 0; i <= 30; i++) {
        gl_Position = gl_in[0].gl_Position + vec4(cos(i * 2.0f * Pi / 20.0f) * radius, sin(i * 2.0f * Pi / 20.0f) * radius, 0.0, 0.0);
        EmitVertex();
    }
    EndPrimitive();
}