#version 330 core
layout(location = 0) in int aPieceRow;
layout(location = 1) in int aPieceCol;

uniform float u_aspect;

void main() {
  float cx = mix(-0.9, 0.9, float(aPieceCol) / 8.0) * u_aspect;
  float cy = mix(0.9, -0.9, float(aPieceRow) / 9.0);
  gl_Position = vec4(cx, cy, -0.2, 1.0);
}
