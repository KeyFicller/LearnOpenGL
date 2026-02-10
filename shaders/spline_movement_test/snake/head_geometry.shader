#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 120) out;

in vec3 vs_position[];

uniform vec3 uHeadDirection;
uniform float uHeadSize;
uniform float uOffsetRatio;

void main() {
    vec3 centerPos = vs_position[0];
    vec3 headDir = normalize(uHeadDirection);
    vec3 perpDir = vec3(-headDir.y, headDir.x, 0.0);
    
    float pi = 3.1415926535897932384626433832795;

    vec3 tip = centerPos + headDir * uHeadSize * uOffsetRatio;
    vec3 leftBase = centerPos - headDir * uHeadSize * 0.3f + perpDir * uHeadSize * 0.5f * uOffsetRatio;
    vec3 rightBase = centerPos - headDir * uHeadSize * 0.3f - perpDir * uHeadSize * 0.5f * uOffsetRatio;
    
    gl_Position = vec4(tip, 1.0);
    EmitVertex();
    
    gl_Position = vec4(leftBase, 1.0);
    EmitVertex();
    
    gl_Position = vec4(rightBase, 1.0);
    EmitVertex();
}
