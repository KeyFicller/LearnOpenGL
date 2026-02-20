#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int FragPieceId;

uniform vec4 u_fill_color;

flat in int gs_piece_id;

void main() {
  FragColor = u_fill_color;
  FragPieceId = gs_piece_id;
}
