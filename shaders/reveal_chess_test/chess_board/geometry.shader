#version 330 core
layout(points) in;
layout(triangle_strip, max_vertices = 120) out;

flat out int gs_board_line;

const float line_half = 0.003;

void draw_horizontal_line(vec2 start, vec2 end) {
  gs_board_line = 1;
  gl_Position = vec4(start.x, start.y - line_half, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(end.x, end.y - line_half, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(start.x, start.y + line_half, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(end.x, end.y + line_half, 0.0, 1.0);
  EmitVertex();
  EndPrimitive();
}

void draw_vertical_line(vec2 start, vec2 end) {
  gs_board_line = 1;
  gl_Position = vec4(start.x - line_half, start.y, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(end.x - line_half, end.y, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(start.x + line_half, start.y, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(end.x + line_half, end.y, 0.0, 1.0);
  EmitVertex();
  EndPrimitive();
}

// For diagonal lines
void draw_line(vec2 start, vec2 end) {
  vec2 d = end - start;
  float len = length(d);
  vec2 perp = (len > 0.001) ? vec2(-d.y, d.x) / len * line_half : vec2(line_half, 0.0);
  gs_board_line = 3;
  gl_Position = vec4(start - perp, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(start + perp, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(end + perp, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(end - perp, 0.0, 1.0);
  EmitVertex();
  EndPrimitive();
}

void main() {
  // Board background
    gl_Position = gl_in[0].gl_Position + vec4(-1.0, -1.0, 0.1, 0.0);
    gs_board_line = 0;
    EmitVertex();
    gl_Position = gl_in[0].gl_Position + vec4(1.0, -1.0, 0.1, 0.0);
    EmitVertex();
    gl_Position = gl_in[0].gl_Position + vec4(-1.0, 1.0, 0.1, 0.0);
    EmitVertex();
    gl_Position = gl_in[0].gl_Position + vec4(1.0, 1.0, 0.1, 0.0);
    EmitVertex();
  EndPrimitive();

  vec2 left_up = vec2(-0.9, 0.9);
  vec2 right_up = vec2(0.9, 0.9);
  vec2 left_down = vec2(-0.9, -0.9);
  vec2 right_down = vec2(0.9, -0.9);

  // Grid lines
  for (int i = 0; i <= 9; i++) {
    draw_horizontal_line(mix(left_up, left_down, float(i) / 9.0), mix(right_up, right_down, float(i) / 9.0));
  }

  for (int i = 0; i <= 8; i++) {
    draw_vertical_line(mix(left_up, right_up, float(i) / 8.0), mix(left_down, right_down, float(i) / 8.0));
  }

  // River
  float river_y_top = mix(0.9, -0.9, 4.0 / 9.0);
  float river_y_bot = mix(0.9, -0.9, 5.0 / 9.0);
  gs_board_line = 2;
  gl_Position = vec4(-0.9, river_y_top, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(0.9, river_y_top, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(-0.9, river_y_bot, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(0.9, river_y_bot, 0.0, 1.0);
  EmitVertex();
  EndPrimitive();

  // House X
  float pal_x_l = mix(-0.9, 0.9, 3.0 / 8.0);
  float pal_x_r = mix(-0.9, 0.9, 5.0 / 8.0);
  float top_y = mix(0.9, -0.9, 0.0 / 9.0);
  float top_mid_y = mix(0.9, -0.9, 2.0 / 9.0);
  draw_line(vec2(pal_x_l, top_y), vec2(pal_x_r, top_mid_y));
  draw_line(vec2(pal_x_r, top_y), vec2(pal_x_l, top_mid_y));
  float bot_y = mix(0.9, -0.9, 9.0 / 9.0);
  float bot_mid_y = mix(0.9, -0.9, 7.0 / 9.0);
  draw_line(vec2(pal_x_l, bot_mid_y), vec2(pal_x_r, bot_y));
  draw_line(vec2(pal_x_r, bot_mid_y), vec2(pal_x_l, bot_y));
}