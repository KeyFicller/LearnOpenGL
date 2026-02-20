#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int FragPieceId;

flat in int gs_piece_type;
flat in int gs_piece_id;
void main() {
  const int red_mask = 0x00200;
  const int black_mask = 0x00400;
  const int cover_mask = 0x00100;
  FragPieceId = gs_piece_id;
  if ((gs_piece_type & cover_mask) != 0) {
    FragColor = vec4(0.6, 0.6, 0.6, 1.0);
  }
  else if ((gs_piece_type & red_mask) != 0) {
    FragColor = vec4(0.6, 0.2, 0.2, 1.0);
  } else if ((gs_piece_type & black_mask) != 0) {
    FragColor = vec4(0.2, 0.2, 0.6, 1.0);
  } else {
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
  }
}
