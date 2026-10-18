#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out uint g_ObjectId; // -1 (no object) for framebuffer picking

in vec2 gs_pos;

// Board space: x is divided by this so the grid stays square on a wide viewport.
uniform float u_aspect;

// The board is one quad; the grid, the river, the palaces and the frame are all
// distances in the fragment shader. Geometry-drawn lines could only be 3 px
// bands with hard edges.
const float k_half = 0.9;               // grid extent in NDC
const vec2 k_cell = vec2(0.225, 0.200); // column and row spacing: 1.8/8, 1.8/9
const float k_line_half = 0.0028;       // half the width of a grid line
const float k_river = 0.100;            // the bank sits between rows 4 and 5
const float k_frame = 0.960;            // outer edge of the wooden surround

const vec3 k_wood = vec3(0.850, 0.730, 0.555);
const vec3 k_wood_dark = vec3(0.795, 0.665, 0.495);
const vec3 k_wood_light = vec3(0.900, 0.790, 0.620);
const vec3 k_ink = vec3(0.175, 0.100, 0.055);
const vec3 k_river_tint = vec3(0.700, 0.745, 0.780);
const vec3 k_frame_dark = vec3(0.320, 0.195, 0.105);
const vec3 k_frame_light = vec3(0.560, 0.395, 0.235);
const vec3 k_bevel = vec3(0.760, 0.580, 0.350);

float hash21(vec2 p) {
  p = fract(p * vec2(123.34, 456.21));
  p += dot(p, p + 45.32);
  return fract(p.x * p.y);
}

float value_noise(vec2 p) {
  vec2 i = floor(p);
  vec2 f = fract(p);
  vec2 u = f * f * (3.0 - 2.0 * f);
  float a = hash21(i);
  float b = hash21(i + vec2(1.0, 0.0));
  float c = hash21(i + vec2(0.0, 1.0));
  float d = hash21(i + vec2(1.0, 1.0));
  return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
  float sum = 0.0;
  float amp = 0.5;
  for (int i = 0; i < 3; ++i) {
    sum += amp * value_noise(p);
    p *= 2.07;
    amp *= 0.5;
  }
  return sum;
}

// Distance from p to the segment ab, for the palace diagonals.
float segment_distance(vec2 p, vec2 a, vec2 b) {
  vec2 pa = p - a;
  vec2 ba = b - a;
  float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
  return length(pa - ba * h);
}

void main() {
  g_ObjectId = 0xFFFFFFFFu; // no object
  vec2 p = vec2(gs_pos.x / u_aspect, gs_pos.y);
  float edge = max(abs(p.x), abs(p.y));

  // Wood. Streaks run along the board, plus a slow blotch so the surface is
  // never one flat colour. The frequencies stay low: fine noise would just
  // alias into sandpaper.
  float streak = fbm(vec2(p.x * 9.0, p.y * 2.5));
  float fine = fbm(vec2(p.x * 26.0, p.y * 6.0));
  vec3 wood = mix(k_wood_dark, k_wood_light, smoothstep(0.28, 0.78, streak));
  wood = mix(wood, k_wood, 0.45 * fine);
  wood *= 0.975 + 0.05 * fbm(p * 2.6 + 11.0);

  vec3 col = wood;

  // River: a cool wash over the same wood, not a painted band.
  float bank = 1.0 - smoothstep(k_river - 0.004, k_river + 0.004, abs(p.y));
  col = mix(col, k_river_tint, bank * 0.20);

  // Grid. Vertical lines stop at the river except the two border files, which
  // is how a real board is drawn.
  vec2 g = vec2((p.x + k_half) / k_cell.x, (k_half - p.y) / k_cell.y);
  vec2 to_line = abs(fract(g + 0.5) - 0.5) * k_cell;
  // A constant width per pixel keeps the edge soft without the derivative
  // jumping at the cell wrap, where fract() is discontinuous.
  float aa_x = fwidth(p.x) * 0.50;
  float aa_y = fwidth(p.y) * 0.50;
  float vline = 1.0 - smoothstep(k_line_half - aa_x, k_line_half + aa_x, to_line.x);
  float hline = 1.0 - smoothstep(k_line_half - aa_y, k_line_half + aa_y, to_line.y);
  float column = floor(g.x + 0.5);
  float border_file = (column < 0.5 || column > 7.5) ? 1.0 : 0.0;
  vline *= mix(1.0, border_file, bank);

  // Palaces: one X per side, rows 0-2 and 7-9, columns 3-5.
  float palace = min(
      min(segment_distance(p, vec2(-0.225, 0.9), vec2(0.225, 0.5)),
          segment_distance(p, vec2(0.225, 0.9), vec2(-0.225, 0.5))),
      min(segment_distance(p, vec2(-0.225, -0.5), vec2(0.225, -0.9)),
          segment_distance(p, vec2(0.225, -0.5), vec2(-0.225, -0.9))));
  float palace_line =
      1.0 - smoothstep(k_line_half - aa_x, k_line_half + aa_x, palace);

  // Frame: darker wood around the playing field, lit where it meets the board
  // and falling off toward the outer edge.
  float in_frame = smoothstep(k_half - 0.0015, k_half + 0.0015, edge);
  vec3 frame = mix(k_frame_dark, k_frame_light, 0.30 + 0.55 * streak);
  float bevel_d = (edge - k_half - 0.005) / 0.009;
  frame = mix(frame, k_bevel, exp(-bevel_d * bevel_d) * 0.85);
  frame *= 1.0 - 0.45 * smoothstep(k_frame - 0.010, k_frame, edge);
  col = mix(col, frame, in_frame);

  // Everything past the frame is the table the board sits on, with the board
  // casting a little shadow onto it.
  float off_board = smoothstep(k_frame - 0.002, k_frame + 0.002, edge);
  vec3 table = vec3(0.150, 0.132, 0.118) *
               (0.70 + 0.30 * smoothstep(k_frame, k_frame + 0.10, edge));
  col = mix(col, table, off_board);

  // The frame casts a little shadow onto the board, strongest where they meet.
  col *= 1.0 - 0.05 * smoothstep(k_half - 0.045, k_half - 0.002, edge);

  // Ink last, so the two border lines stay crisp where they meet the frame.
  col = mix(col, k_ink, max(max(vline, hline), palace_line * 0.9));

  FragColor = vec4(col, 1.0);
}
