#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int FragObjectId;

uniform vec3 uBodyColor;

void main() {
  FragColor = vec4(uBodyColor, 1.0);
  FragObjectId = 1;
}
