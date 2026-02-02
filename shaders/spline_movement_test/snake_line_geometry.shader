#version 330 core
layout (lines) in;
layout (triangle_strip, max_vertices = 4) out;

uniform float uLineWidth;

in float vs_progress[];
out float gs_progress;

void main() {
    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    
    vec3 dir = normalize(p1 - p0);
    
    vec3 perp = vec3(-dir.y, dir.x, 0.0) * uLineWidth * (1.0f - vs_progress[0]);
    
    gl_Position = vec4(p0 - perp, 1.0);
    gs_progress = vs_progress[0];
    EmitVertex();
    
    gl_Position = vec4(p0 + perp, 1.0);
    gs_progress = vs_progress[0];
    EmitVertex();


    perp = vec3(-dir.y, dir.x, 0.0) * uLineWidth * (1.0f - vs_progress[1]);
    
    gl_Position = vec4(p1 - perp, 1.0);
    gs_progress = vs_progress[1];
    EmitVertex();
    
    gl_Position = vec4(p1 + perp, 1.0);
    gs_progress = vs_progress[1];
    EmitVertex();
    
    EndPrimitive();
}