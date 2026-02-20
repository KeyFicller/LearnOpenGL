#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out uint g_ObjectId; // -1 (no object) for framebuffer picking

flat in int gs_board_line;

void main() {
  g_ObjectId = 0xFFFFFFFFu; // no object
  if (gs_board_line == 0) {
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
  } else {
    FragColor = vec4(0.0, 0.0, 0.0, 1.0);
  }
}
