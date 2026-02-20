#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int FragPieceId;

flat in int gs_piece_id;

void main() {
  FragColor = vec4(1.0, 0.0, 0.0, 1.0);
  FragPieceId = gs_piece_id;
}
