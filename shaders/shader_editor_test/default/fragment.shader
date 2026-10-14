#version 330 core

out vec4 FragColor;

uniform float u_time;
uniform float u_random;
uniform vec2  u_resolution;
uniform vec2  u_mouse;

// ---------- hash / noise ----------
float hash(vec2 p) {
  p = fract(p * vec2(123.34, 456.21));
  p += dot(p, p + 45.32);
  return fract(p.x * p.y);
}

float vnoise(vec2 p) {
  vec2 i = floor(p), f = fract(p);
  vec2 u = f * f * (3.0 - 2.0 * f);
  return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x),
             mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

const mat2 ROT = mat2(0.80, 0.60, -0.60, 0.80);

float fbm(vec2 p) {
  float v = 0.0, a = 0.5;
  for (int i = 0; i < 6; i++) {
    v += a * vnoise(p);
    p = ROT * p * 2.03;
    a *= 0.5;
  }
  return v;
}

float ridged(vec2 p) {
  float v = 0.0, a = 0.5;
  for (int i = 0; i < 5; i++) {
    v += a * (1.0 - abs(vnoise(p) * 2.0 - 1.0));
    p = ROT * p * 2.11;
    a *= 0.55;
  }
  return v;
}

// ---------- iridescent palette ----------
vec3 palette(float t) {
  return 0.5 + 0.5 * cos(6.28318 * (vec3(1.0, 1.0, 1.0) * t + vec3(0.00, 0.33, 0.67)));
}

vec3 hue(float t) {
  return 0.5 + 0.5 * cos(6.28318 * (t + vec3(0.0, 0.33, 0.67)));
}

// ---------- star layer ----------
vec3 stars(vec2 uv, float t) {
  vec3 acc = vec3(0.0);
  for (int k = 0; k < 3; k++) {
    float fk = float(k);
    vec2 sp = uv * (7.0 + fk * 5.0) + vec2(fk * 13.7, -fk * 8.3);
    vec2 id = floor(sp);
    vec2 gv = fract(sp) - 0.5;
    float h = hash(id + fk * 41.0);
    if (h > 0.86) {
      vec2 off = (vec2(hash(id + 3.1), hash(id + 7.7)) - 0.5) * 0.7;
      float d = length(gv - off);
      float tw = 0.55 + 0.45 * sin(t * (2.0 + h * 6.0) + h * 30.0);
      float s = smoothstep(0.09, 0.0, d) * tw;
      acc += hue(h * 3.0 + 0.1) * s * (0.6 + fk * 0.25);
    }
  }
  return acc;
}

void main() {
  vec2 uv = (gl_FragCoord.xy - 0.5 * u_resolution) / min(u_resolution.x, u_resolution.y);
  float t = u_time * 0.22;

  // mouse pulls a gravity well
  vec2 m = (u_mouse - 0.5) * (u_resolution / min(u_resolution.x, u_resolution.y));
  vec2 center = mix(vec2(0.0), m, 0.55);

  vec2 p = uv - center;
  float r = length(p);
  float ang = atan(p.y, p.x);

  // vortex swirl: bends space more near the well
  ang += 2.0 / (r + 0.30) + t * 1.6;
  vec2 q = vec2(cos(ang), sin(ang)) * r;

  // double domain warp
  vec2 w1 = vec2(fbm(q * 1.8 + t),
                 fbm(q * 1.8 + vec2(5.2, 1.3) - t * 1.2));
  vec2 w2 = vec2(fbm(q * 2.6 + 3.2 * w1 + vec2(1.7, 9.2) + t * 1.4),
                 fbm(q * 2.6 + 3.2 * w1 + vec2(8.3, 2.8) - t * 1.1));
  float f = fbm(q * 2.2 + 2.8 * w2);
  float fil = ridged(q * 4.5 + 2.0 * w2) * 0.5;

  // emission
  vec3 col = palette(f * 1.5 + r * 0.55 + t * 0.30);
  col *= 0.20 + 1.85 * f;
  col += hue(f + 0.42) * pow(fil, 2.2) * 1.1;          // bright filaments
  col += vec3(0.35, 0.65, 1.0) * exp(-r * 2.4) * 0.45; // cool halo

  // pulsating core
  float pulse = 0.75 + 0.25 * sin(u_time * 1.7);
  float core = exp(-r * 13.0) * pulse;
  col += vec3(1.0, 0.90, 0.72) * core * 2.2;
  col += vec3(1.0, 0.55, 0.35) * exp(-r * 5.0) * 0.35;

  // stars, faded toward the bright center
  col += stars(uv + vec2(t * 0.02, -t * 0.05), u_time) * smoothstep(0.08, 0.7, r);

  // vignette + tone map + gamma
  col *= 1.0 - 0.45 * smoothstep(0.35, 1.25, r);
  col = col / (1.0 + col);
  col = pow(max(col, 0.0), vec3(0.4545));

  FragColor = vec4(col, 1.0);
}
