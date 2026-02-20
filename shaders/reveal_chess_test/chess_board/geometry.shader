#version 330 core
layout(points) in;
layout(triangle_strip, max_vertices = 92) out;

flat out int gs_board_line;

void draw_horizontal_line(vec2 start, vec2 end) {
  float offset = 0.002;
  gs_board_line = 1;
  gl_Position = vec4(start.x, start.y - offset, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(end.x, end.y - offset, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(start.x, start.y + offset, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(end.x, end.y + offset, 0.0, 1.0);
  EmitVertex();
  EndPrimitive();
}

void draw_vertical_line(vec2 start, vec2 end) {
  float offset = 0.002;
  gs_board_line = 1;
  gl_Position = vec4(start.x - offset, start.y, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(end.x - offset, end.y, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(start.x + offset, start.y, 0.0, 1.0);
  EmitVertex();
  gl_Position = vec4(end.x + offset, end.y, 0.0, 1.0);
  EmitVertex();
  EndPrimitive();
}

void main() {
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

  // 9 columns => 10 vertical lines; 10 rows => 11 horizontal lines
  vec2 left_up = vec2(-0.9, 0.9);
  vec2 right_up = vec2(0.9, 0.9);
  vec2 left_down = vec2(-0.9, -0.9);
  vec2 right_down = vec2(0.9, -0.9);

  for (int i = 0; i <= 9; i++) {
    draw_horizontal_line(mix(left_up, left_down, float(i) / 9.0), mix(right_up, right_down, float(i) / 9.0));
  }

  for (int i = 0; i <= 8; i++) {
    draw_vertical_line(mix(left_up, right_up, float(i) / 8.0), mix(left_down, right_down, float(i) / 8.0));
  }
}