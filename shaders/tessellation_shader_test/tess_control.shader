#version 400 core
layout(vertices = 3) out;

uniform float uOuterLevel;
uniform float uInnerLevel;

void main() {
  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

  if (gl_InvocationID == 0) {
    gl_TessLevelOuter[0] = uOuterLevel;
    gl_TessLevelOuter[1] = uOuterLevel;
    gl_TessLevelOuter[2] = uOuterLevel;
    gl_TessLevelInner[0] = uInnerLevel;
  }
}
