#version 330 core
layout(points) in;
layout(triangle_strip, max_vertices = 40) out;

const float half_w = 0.102;
const float half_h = 0.092;
const float line_half = 0.006;

void emit_line(vec2 a, vec2 b, float z) {
  vec2 d = b - a;
  float len = length(d);
  vec2 perp = (len > 0.001) ? vec2(-d.y, d.x) / len * line_half : vec2(line_half, 0.0);
  gl_Position = vec4(a - perp, z, 1.0);
  EmitVertex();
  gl_Position = vec4(a + perp, z, 1.0);
  EmitVertex();
  gl_Position = vec4(b + perp, z, 1.0);
  EmitVertex();
  gl_Position = vec4(b - perp, z, 1.0);
  EmitVertex();
  EndPrimitive();
}

void main() {
  vec2 center = gl_in[0].gl_Position.xy;
  float z = gl_in[0].gl_Position.z;
  float l = center.x - half_w;
  float r = center.x + half_w;
  float b = center.y - half_h;
  float t = center.y + half_h;
  emit_line(vec2(l, b), vec2(r, b), z);
  emit_line(vec2(r, b), vec2(r, t), z);
  emit_line(vec2(r, t), vec2(l, t), z);
  emit_line(vec2(l, t), vec2(l, b), z);
}
