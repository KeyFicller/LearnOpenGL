#version 330 core

in vec2 vUv;
out vec4 FragColor;
uniform sampler2D u_texture;

void main() { FragColor = vec4(texture(u_texture, vUv).rgb, 1.0); }
