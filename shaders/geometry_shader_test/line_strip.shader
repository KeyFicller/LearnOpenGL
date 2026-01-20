#version 330 core

layout (points) in;

layout(line_strip, max_vertices = 2) out;

uniform float uShapeFactor;

void main() {
  gl_Position = gl_in[0].gl_Position + vec4(0.0, -uShapeFactor / 2.0f, 0.0, 0.0);
  EmitVertex();
  gl_Position = gl_in[0].gl_Position + vec4(0.0, uShapeFactor / 2.0f, 0.0, 0.0);
  EmitVertex();
  EndPrimitive();
}