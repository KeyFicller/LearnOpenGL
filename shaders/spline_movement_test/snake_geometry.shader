#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 60) out;

uniform int uTotalPoints;
uniform int uDrawingHead;
uniform float uDrawingHeadRadius;

in vec3 vs_position[];
out vec4 gs_color;
out float gs_segment_progress;

void main() {
    // Calculate progress along the snake (0.0 at head, 1.0 at tail)
    // Use gl_PrimitiveIDIn to get the current point index
    float progress = (uTotalPoints > 1) ? (float(gl_PrimitiveIDIn) / float(uTotalPoints - 1)) : 0.0;
    
    // Radius decreases from head to tail
    float headRadius = 0.03f;
    float tailRadius = 0.01f;
    float segmentRadius = mix(headRadius, tailRadius, progress);
    
    int segments = 16; // Number of vertices per segment circle
    
    // Color gradient: bright green at head, darker green at tail
    vec3 headColor = vec3(0.3, 1.0, 0.4);
    vec3 tailColor = vec3(0.1, 0.6, 0.2);
    vec3 segmentColor = mix(headColor, tailColor, progress);
    
    // Add slight glow at the head
    if (progress < 0.2) {
        segmentColor += vec3(0.2, 0.3, 0.2) * (1.0 - progress * 5.0);
    }
    
    float pi = 3.1415926535897932384626433832795;
    
    if (uDrawingHead == 1) {

        float drawingHeadRadius = uDrawingHeadRadius * 1.15f;
        // Draw filled circle for head: triangle fan from center
        // For triangle_strip: center, p0, p1, center, p2, p1, center, p3, p2, ...
        vec3 centerPos = vs_position[0];
        
        // Emit center and first two points
        gl_Position = vec4(centerPos, 1.0);
        gs_color = vec4(segmentColor, 1.0);
        gs_segment_progress = progress;
        EmitVertex();
        
        float angle0 = 0.0;
        vec3 offset0 = vec3(cos(angle0) * drawingHeadRadius, sin(angle0) * drawingHeadRadius, 0.0);
        gl_Position = vec4(centerPos + offset0, 1.0);
        gs_color = vec4(segmentColor, 1.0);
        gs_segment_progress = progress;
        EmitVertex();
        
        // Emit remaining vertices in triangle_strip pattern for triangle fan
        for (int i = 1; i <= segments; i++) {
            float angle = (float(i) / float(segments)) * 2.0 * pi;
            vec3 offset = vec3(cos(angle) * drawingHeadRadius, sin(angle) * drawingHeadRadius, 0.0);
            
            // Emit center, then previous point, then current point
            gl_Position = vec4(centerPos, 1.0);
            gs_color = vec4(segmentColor, 1.0);
            gs_segment_progress = progress;
            EmitVertex();
            
            // Previous point
            float prevAngle = (float(i - 1) / float(segments)) * 2.0 * pi;
            vec3 prevOffset = vec3(cos(prevAngle) * drawingHeadRadius, sin(prevAngle) * drawingHeadRadius, 0.0);
            gl_Position = vec4(centerPos + prevOffset, 1.0);
            gs_color = vec4(segmentColor, 1.0);
            gs_segment_progress = progress;
            EmitVertex();
            
            // Current point
            gl_Position = vec4(centerPos + offset, 1.0);
            gs_color = vec4(segmentColor, 1.0);
            gs_segment_progress = progress;
            EmitVertex();
        }
        EndPrimitive();
    } else {
        // Generate circle outline for body segments
        for (int i = 0; i <= segments; i++) {
            float angle = (float(i) / float(segments)) * 2.0 * pi;
            vec3 offset = vec3(cos(angle) * segmentRadius, sin(angle) * segmentRadius, 0.0);
            gl_Position = vec4(vs_position[0] + offset, 1.0);
            gs_color = vec4(segmentColor, 1.0);
            gs_segment_progress = progress;
            EmitVertex();
        }
        EndPrimitive();
    }
}