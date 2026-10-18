#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int FragPieceId;

uniform int uSelectedPieceId;

flat in int gs_piece_type;
flat in int gs_piece_id;
flat in int gs_shadow;
in vec2 gs_local; // center=0, rim=1

// A disc with an ivory edge, so a piece reads as an object sitting on the wood
// instead of a coloured circle. The cover is a wooden back: no red or black hint
// anywhere on it.
const vec3 k_ivory = vec3(0.930, 0.885, 0.780);
const vec3 k_ivory_shade = vec3(0.705, 0.625, 0.490);
const vec3 k_face_red = vec3(0.760, 0.200, 0.140);
const vec3 k_face_black = vec3(0.145, 0.145, 0.190);
const vec3 k_face_cover = vec3(0.505, 0.420, 0.330);
const vec3 k_gold = vec3(1.000, 0.855, 0.450);

void main() {
  float d = length(gs_local);
  FragPieceId = gs_piece_id;
  if (d > 1.0)
    discard;

  if (gs_shadow == 1) {
    // A soft contact shadow. Only the part past the piece rim is visible, so
    // the falloff sits at the shadow's own edge.
    FragColor = vec4(0.120, 0.075, 0.045,
                     0.34 * (1.0 - smoothstep(0.88, 1.0, d)));
    return;
  }

  const int black_mask = 0x00400;
  const int cover_mask = 0x00100;

  bool covered = (gs_piece_type & cover_mask) != 0;
  bool black = !covered && (gs_piece_type & black_mask) != 0;

  vec3 face = covered ? k_face_cover : (black ? k_face_black : k_face_red);

  // Ivory edge, and a groove just inside it so the face looks set into the rim.
  float rim = smoothstep(0.815, 0.835, d);
  float groove = smoothstep(0.775, 0.800, d) * (1.0 - rim);
  vec3 col = mix(face, k_ivory, rim);
  col = mix(col, k_ivory_shade, groove * 0.55);

  // Light from the upper left: the rim picks it up, the face gets a gentle dome.
  vec2 to_edge = gs_local + vec2(1e-4, 1e-4);
  float lift = 0.5 + 0.5 * dot(normalize(to_edge), normalize(vec2(-0.55, 0.80)));
  col *= 0.93 + 0.13 * lift;
  col *= 1.0 - 0.10 * d * d;

  // Covered pieces get an engraved ring and a dot: a back, not a blank disc.
  if (covered) {
    float ring = smoothstep(0.520, 0.545, d) * (1.0 - smoothstep(0.575, 0.600, d));
    col = mix(col, k_face_cover * 0.66, ring * 0.60);
    col = mix(col, k_face_cover * 0.66, (1.0 - smoothstep(0.095, 0.125, d)) * 0.45);
  }

  if (gs_piece_id == uSelectedPieceId) {
    // Gold rim plus a slight overall lift: the piece is picked up, not enlarged
    // into the neighbouring squares.
    col = mix(col, k_gold, smoothstep(0.885, 1.0, d) * 0.80);
    col = mix(col, vec3(1.0), 0.05);
  }

  FragColor = vec4(col, 1.0);
}
