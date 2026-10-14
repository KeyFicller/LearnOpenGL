#version 330 core

in vec3 vPos;
out vec4 FragColor;

uniform float u_time;
uniform vec2  u_mouse;
uniform vec2  u_resolution;

float hash21(vec2 p) {
  p = fract(p * vec2(123.34, 456.21));
  p += dot(p, p + 45.32);
  return fract(p.x * p.y);
}

mat2 rot(float a) { float c = cos(a), s = sin(a); return mat2(c, -s, s, c); }

vec3 palette(float t) {
  return 0.5 + 0.5 * cos(6.28318 * (t + vec3(0.00, 0.33, 0.67)));
}

float sdRoundBox(vec2 p, vec2 b, float r) {
  vec2 q = abs(p) - b + r;
  return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

vec3 tileLayer(vec2 p, vec2 m, float tiles, float t, float seed, float sizeMul) {
  vec2 sp = p * tiles;
  vec2 id = floor(sp);
  vec2 lp = fract(sp) - 0.5;

  float h  = hash21(id + seed);
  float h2 = hash21(id * 1.73 + seed + 9.1);

  float mdist = length(id + 0.5 - m * tiles);
  float mglow = exp(-mdist * 1.1);

  float ang = h * 6.28318 + t * (0.35 + h2 * 0.9) * (0.5 + 2.2 * mglow);
  vec2 q = rot(ang) * lp;

  float ripple = 0.05 * sin(t * 1.8 - length(p) * 9.0);
  float sz = (0.20 + 0.11 * h) * sizeMul * (1.0 + 0.15 * sin(t * 2.0 + h * 21.0) + ripple);

  float d  = sdRoundBox(q, vec2(sz), 0.06 + 0.05 * h2);
  float aa = max(fwidth(d), 1e-4) * 1.2;
  float fill = 1.0 - smoothstep(-aa, aa, d);
  float rim  = exp(-abs(d) * (55.0 + 40.0 * h2));

  vec3 col = palette(h * 0.9 + t * 0.10 + 0.12 * mglow);
  col *= 0.30 + 0.85 * h2;
  col += vec3(1.0) * rim * 0.7;
  return col * fill + col * rim * (1.0 + 2.0 * mglow);
}

void main() {
  float f = min(u_resolution.x, u_resolution.y);
  vec2 p = (gl_FragCoord.xy - 0.5 * u_resolution) / f;
  vec2 m = (u_mouse - 0.5) * (u_resolution / f);
  float t = u_time * 0.6;

  vec2 q = rot(0.15 * sin(t * 0.25)) * p;

  vec3 col = vec3(0.0);
  col += tileLayer(q + 0.011, m, 6.0, t, 0.0, 1.0);
  col += tileLayer(q * 1.03 - 0.017, m, 12.0, t, 31.7, 0.55) * 0.7;

  col += palette(t * 0.05) * 0.03;
  col += vec3(0.35, 0.6, 1.0) * exp(-length(p - m) * 5.0) * 0.10;

  col *= 1.0 - 0.45 * smoothstep(0.25, 1.0, length(p));
  col = col / (1.0 + col);
  col = pow(max(col, 0.0), vec3(0.4545));

  FragColor = vec4(col, 1.0);
}
