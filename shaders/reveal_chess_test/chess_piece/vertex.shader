#version 330 core
layout(location = 0) in int aPieceRow;
layout(location = 1) in int aPieceCol;
layout(location = 2) in int aPieceType;
layout(location = 3) in int aPieceId;

// x is scaled so a board-space unit is the same number of pixels on both axes.
uniform float u_aspect;

flat out int vs_piece_type;
flat out int vs_piece_id;

void main() {
  gl_Position =
      vec4(mix(-0.9, 0.9, float(aPieceCol) / 8.0) * u_aspect,
           mix(0.9, -0.9, float(aPieceRow) / 9.0), -0.1, 1.0);
  vs_piece_type = aPieceType;
  vs_piece_id = aPieceId;
}
