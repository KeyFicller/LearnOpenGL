#version 330 core

in vec3 vPos;
in vec2 vUv;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

float line(float a, float b, float width, float edge)
{
    float half_width = 0.5 * width;
    return smoothstep(a - half_width - edge, a - half_width, b) - 
        smoothstep(a + half_width, a + half_width + edge, b);
}

void main()
{
    FragColor = vec4(
      vec3(1.0, 0.0, 0.0) * line(0.5 * vPos.x, vPos.y, 0.01, 0.001) +  // y = 0.5 * x, centered at {0,0}
      vec3(0.0, 1.0, 0.0) * line(2 * gl_FragCoord.x, gl_FragCoord.y, 10, 1) + // y = 2 * x, centered at pixel {0,0}
      vec3(0.0, 0.0, 1.0) * line(vPos.y, sin(vPos.x * 3.1415926), 0.02, 0.001) + // y = sin(2pi * x) 
      vec3(1.0, 1.0, 1.0) * line(vUv.y, 0.5 * vUv.x, 0.01, 0.001)
    , 1.0);
}
