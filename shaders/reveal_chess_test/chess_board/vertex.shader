#version 330 core
layout(location = 0) in vec3 aPos;

void main() {
  gl_Position.x = aPos.x;
  gl_Position.y = aPos.y;
  gl_Position.z = aPos.z;
  gl_Position.w = 1.0;
}
