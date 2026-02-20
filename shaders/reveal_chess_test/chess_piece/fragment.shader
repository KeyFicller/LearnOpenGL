#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int FragPieceId;

uniform int uSelectedPieceId;

flat in int gs_piece_type;
flat in int gs_piece_id;
in vec2 gs_local; // interpolated: center=0, edge length=1

// Base colors
const vec3 piece_red = vec3(0.78, 0.22, 0.16);
const vec3 piece_black = vec3(0.12, 0.12, 0.18);
const vec3 piece_cover = vec3(0.55, 0.50, 0.48);
const vec3 outline_dark = vec3(0.08, 0.06, 0.05);
const vec3 outline_light = vec3(0.95, 0.92, 0.88);
const vec3 outline_cover = vec3(0.35, 0.32, 0.30); // neutral, no red/black hint

void main() {
  const int red_mask = 0x00200;
  const int black_mask = 0x00400;
  const int cover_mask = 0x00100;
  FragPieceId = gs_piece_id;

  float dist = length(gs_local);
  if (dist > 1.0)
    discard;

  vec3 base;
  if ((gs_piece_type & cover_mask) != 0) {
    base = piece_cover;
  } else if ((gs_piece_type & red_mask) != 0) {
    base = piece_red;
  } else if ((gs_piece_type & black_mask) != 0) {
    base = piece_black;
  } else {
    base = vec3(1.0);
  }

  // Rim: dark for red, light for black, neutral for covered (don't reveal color)
  float rim = smoothstep(0.72, 0.88, dist);
  vec3 rim_color;
  if ((gs_piece_type & cover_mask) != 0)
    rim_color = outline_cover;
  else if ((gs_piece_type & black_mask) != 0)
    rim_color = outline_light;
  else
    rim_color = outline_dark;
  vec3 col = mix(base, rim_color, rim * 0.9);

  // Slight radial gradient: lighter center, darker edge (domed feel)
  float shade = 1.0 - 0.22 * dist;
  col *= shade;

  // Highlight for selected piece: brighten and soft rim glow
  if (gs_piece_id == uSelectedPieceId) {
    col = mix(col, vec3(1.0), 0.18);
    float highlight_rim = smoothstep(0.65, 0.95, dist);
    col = mix(col, vec3(1.0, 0.98, 0.92), highlight_rim * 0.35);
  }

  FragColor = vec4(col, 1.0);
}
