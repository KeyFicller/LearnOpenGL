#version 330 core

out vec4 FragColor;
uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;

void main() {
  FragColor = vec4((sin(u_time)+1.0)*0.5,
   (cos(u_time)+1.0)*0.5, 0.0, 1.0);
}









