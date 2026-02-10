#version 330 core
layout (lines) in;
layout (triangle_strip, max_vertices = 4) out;

uniform float uLineWidth;
uniform vec3 uShapeFactor;
uniform float uOffsetRatio;
in float vs_progress[];
out float gs_progress;

void main() {
    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;

    float ratio = uShapeFactor.x * vs_progress[0] * vs_progress[0] + uShapeFactor.y * vs_progress[0] + uShapeFactor.z;
    ratio = ratio * uOffsetRatio;

    vec3 dir = normalize(p1 - p0);
    float length = length(p1 - p0);
    
    vec3 perp = vec3(-dir.y, dir.x, 0.0) * uLineWidth * (1.0f - vs_progress[0]) * ratio;
    
    gl_Position = vec4(p0 - dir * length * 0.2f - perp, 1.0);
    gs_progress = vs_progress[0];
    EmitVertex();
    
    gl_Position = vec4(p0 - dir * length * 0.2f + perp, 1.0);
    gs_progress = vs_progress[0];
    EmitVertex();


    ratio = uShapeFactor.x * vs_progress[1] * vs_progress[1] + uShapeFactor.y * vs_progress[1] + uShapeFactor.z;
    ratio = ratio * uOffsetRatio;
    
    perp = vec3(-dir.y, dir.x, 0.0) * uLineWidth * (1.0f - vs_progress[1]) * ratio;
    
    gl_Position = vec4(p1 + dir * length * 0.2f - perp, 1.0);
    gs_progress = vs_progress[1];
    EmitVertex();
    
    gl_Position = vec4(p1 + dir * length * 0.2f + perp, 1.0);
    gs_progress = vs_progress[1];
    EmitVertex();
    
    EndPrimitive();
}