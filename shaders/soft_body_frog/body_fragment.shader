#version 330 core
out vec4 FragColor;

uniform vec3 uBodyColor;

void main() {
  FragColor = vec4(uBodyColor, 1.0);
}
