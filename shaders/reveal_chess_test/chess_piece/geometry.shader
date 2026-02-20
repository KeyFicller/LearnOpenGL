#version 330 core
layout(points) in;
layout(triangle_strip, max_vertices = 33) out;

uniform int uHoveredPieceId;
uniform int uSelectedPieceId;

flat in int vs_piece_type[];
flat in int vs_piece_id[];

flat out int gs_piece_type;
flat out int gs_piece_id;
void main() {
  float pi = 3.1415926535897932384626433832795;
  int segments = 16;
  vec3 center = gl_in[0].gl_Position.xyz;
  if (uHoveredPieceId == vs_piece_id[0]) {
    center.y += 0.01;
  }
  float radius = 0.08;
  if (uSelectedPieceId == vs_piece_id[0]) {
    radius *= 1.2;
  }
  int piece_type_val = vs_piece_type[0];
  int piece_id_val = vs_piece_id[0];
  // Filled circle as triangle_strip fan: center, p0, p1, center, p1, p2, ...
  for (int i = 0; i <= segments; i++) {
    float angle = (float(i) / float(segments)) * 2.0 * pi;
    vec3 o = vec3(cos(angle) * radius, sin(angle) * radius, 0.0);
    if (i == 0) {
      gs_piece_type = piece_type_val;
      gs_piece_id = piece_id_val;
      gl_Position = vec4(center, 1.0);
      EmitVertex();
    }
    gs_piece_type = piece_type_val;
    gs_piece_id = piece_id_val;
    gl_Position = vec4(center + o, 1.0);
    EmitVertex();
    if (i > 0 && i < segments) {
      gs_piece_type = piece_type_val;
      gs_piece_id = piece_id_val;
      gl_Position = vec4(center, 1.0);
      EmitVertex();
    }
  }
  EndPrimitive();
}
