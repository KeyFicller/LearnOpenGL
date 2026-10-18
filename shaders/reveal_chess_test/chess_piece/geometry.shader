#version 330 core
layout(points) in;
layout(triangle_strip, max_vertices = 120) out;

uniform int uHoveredPieceId;
uniform int uSelectedPieceId;
uniform float u_aspect; // x scale that keeps the board's proportions

flat in int vs_piece_type[];
flat in int vs_piece_id[];

flat out int gs_piece_type;
flat out int gs_piece_id;
flat out int gs_shadow; // 1 = the contact shadow under a piece
out vec2 gs_local;      // [-1, 1] from center to rim

const float k_pi = 3.141592653589793;
const int k_segments = 24;
const float k_radius = 0.077;
const float k_hover_lift = 0.012;

void emit_disc(vec3 center, float radius, int type, int id, int shadow) {
  // Radius in x is scaled so the disc stays circular on a wide viewport.
  vec2 extent = vec2(radius * u_aspect, radius);
  for (int i = 0; i <= k_segments; ++i) {
    float angle = float(i) / float(k_segments) * 2.0 * k_pi;
    vec2 dir = vec2(cos(angle), sin(angle));
    gs_piece_type = type;
    gs_piece_id = id;
    gs_shadow = shadow;
    if (i == 0) {
      gs_local = vec2(0.0);
      gl_Position = vec4(center, 1.0);
      EmitVertex();
    }
    gs_local = dir;
    gl_Position = vec4(center.xy + dir * extent, center.z, 1.0);
    EmitVertex();
    if (i > 0 && i < k_segments) {
      gs_local = vec2(0.0);
      gl_Position = vec4(center, 1.0);
      EmitVertex();
    }
  }
  EndPrimitive();
}

void main() {
  vec3 center = gl_in[0].gl_Position.xyz;
  int id = vs_piece_id[0];
  int type = vs_piece_type[0];
  float radius = k_radius;
  if (uHoveredPieceId == id)
    center.y += k_hover_lift;
  if (uSelectedPieceId == id)
    radius *= 1.07;

  // Contact shadow, a touch down and to the right, and a hair further from the
  // camera so the piece itself wins the depth test where they overlap. The
  // piece covers the middle, so what shows is the crescent past its rim: the
  // shadow has to stay dark out to d = 1 and only fade at its own edge.
  emit_disc(vec3(center.xy + vec2(0.008 * u_aspect, -0.014), center.z + 0.004),
            radius * 1.06, type, -1, 1);
  emit_disc(center, radius, type, id, 0);
}
