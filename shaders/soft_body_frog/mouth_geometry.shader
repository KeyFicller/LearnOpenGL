#version 330 core
layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

uniform float uLineWidth;

void main() {
  vec4 p0 = gl_in[0].gl_Position;
  vec4 p1 = gl_in[1].gl_Position;
  vec2 dir = normalize((p1.xy / p1.w) - (p0.xy / p0.w));
  vec2 perp = vec2(-dir.y, dir.x) * uLineWidth;

  gl_Position = vec4(p0.xy + perp * p0.w, p0.z, p0.w);
  EmitVertex();
  gl_Position = vec4(p0.xy - perp * p0.w, p0.z, p0.w);
  EmitVertex();
  gl_Position = vec4(p1.xy + perp * p1.w, p1.z, p1.w);
  EmitVertex();
  gl_Position = vec4(p1.xy - perp * p1.w, p1.z, p1.w);
  EmitVertex();
  EndPrimitive();
}
