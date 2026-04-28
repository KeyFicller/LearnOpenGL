#version 400 core
layout(triangles, equal_spacing, ccw) in;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float uTime;
uniform float uAmplitude;

void main() {
  vec3 p0 = gl_in[0].gl_Position.xyz;
  vec3 p1 = gl_in[1].gl_Position.xyz;
  vec3 p2 = gl_in[2].gl_Position.xyz;

  vec3 position = p0 * gl_TessCoord.x + p1 * gl_TessCoord.y + p2 * gl_TessCoord.z;
  position.z += sin((position.x + position.y) * 8.0 + uTime * 2.0) * uAmplitude;

  gl_Position = projection * view * model * vec4(position, 1.0);
}
