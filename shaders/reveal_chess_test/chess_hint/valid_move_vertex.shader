#version 330 core
layout(location = 0) in int aPieceRow;
layout(location = 1) in int aPieceCol;

flat out int vs_piece_id;

void main() {
    gl_Position = vec4(mix(-0.9, 0.9, float(aPieceCol) / 8.0), mix(0.9, -0.9, float(aPieceRow) / 9.0), -0.2, 1.0);
    vs_piece_id = aPieceRow * 10 + aPieceCol;
}
