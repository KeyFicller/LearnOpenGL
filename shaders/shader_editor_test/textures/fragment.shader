#version 330 core

in vec3 vPos;
in vec2 vUv;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_mouse;
uniform vec2 u_resolution;
uniform sampler2D u_texture;

void main()
{
    vec2 p = vPos.xy;
    float len = length(p);
    vec2 ripple = vUv + p/ len * 0.03 * cos(len * 12.0 - u_time * 4.0);
    float delta = (sin(mod(u_time, 8.0) * (2.0 * 3.1415926 / 8.0)) + 1.0) * 0.5;
    vec2 uv = mix(ripple, vUv, 0.7);
    vec3 color = texture(u_texture, uv).rgb;
    
    FragColor = vec4(color, 1.0);
}
