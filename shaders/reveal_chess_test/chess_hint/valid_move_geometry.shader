#version 330 core
layout(points) in;
layout(triangle_strip, max_vertices = 33) out;

uniform float u_radius;

flat in int vs_piece_id[];

flat out int gs_piece_id;

void main() {
  float pi = 3.1415926535897932384626433832795;
  int segments = 16;
  vec3 center = gl_in[0].gl_Position.xyz;
  float radius = u_radius;
  // Filled circle as triangle_strip fan: center, p0, p1, center, p1, p2, ...
  for (int i = 0; i <= segments; i++) {
    float angle = (float(i) / float(segments)) * 2.0 * pi;
    vec3 o = vec3(cos(angle) * radius, sin(angle) * radius, 0.0);
    if (i == 0) {
      gl_Position = vec4(center, 1.0);
      gs_piece_id = vs_piece_id[0];
      EmitVertex();
    }
    gl_Position = vec4(center + o, 1.0);
    EmitVertex();
    if (i > 0 && i < segments) {
      gl_Position = vec4(center, 1.0);
      gs_piece_id = vs_piece_id[0];
      EmitVertex();
    }
  }
  EndPrimitive();
}
