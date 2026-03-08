#version 330 core
layout(points) in;
layout(triangle_strip, max_vertices = 99) out;

uniform float uRadius;

void main() {
  vec4 center = gl_in[0].gl_Position;
  int segments = 32;
  for (int i = 0; i < segments; i++) {
    gl_Position = center;
    EmitVertex();
    float a0 = (float(i) / float(segments)) * 6.28318530718;
    float a1 = (float(i + 1) / float(segments)) * 6.28318530718;
    gl_Position = center + vec4(uRadius * vec2(cos(a0), sin(a0)), 0.0, 0.0);
    EmitVertex();
    gl_Position = center + vec4(uRadius * vec2(cos(a1), sin(a1)), 0.0, 0.0);
    EmitVertex();
    EndPrimitive();
  }
}
