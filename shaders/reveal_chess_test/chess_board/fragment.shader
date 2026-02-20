#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out uint g_ObjectId; // -1 (no object) for framebuffer picking

flat in int gs_board_line;

// Warm wood board: cream/tan background, dark brown grid lines
const vec3 board_bg = vec3(0.82, 0.71, 0.55);   // #D1B58C
const vec3 board_line = vec3(0.22, 0.14, 0.08); // #381415
const vec3 river_color = vec3(0.55, 0.62, 0.68); // 河: slightly cooler/darker band
const vec3 palace_x_color = vec3(0.22, 0.14, 0.08); // same as grid (仕 X)

void main() {
  g_ObjectId = 0xFFFFFFFFu; // no object
  if (gs_board_line == 0) {
    FragColor = vec4(board_bg, 1.0);
  } else if (gs_board_line == 1) {
    FragColor = vec4(board_line, 1.0);
  } else if (gs_board_line == 2) {
    FragColor = vec4(river_color, 1.0);
  } else {
    FragColor = vec4(palace_x_color, 1.0);
  }
}
