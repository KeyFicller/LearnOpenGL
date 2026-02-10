#version 330 core
out vec4 FragColor;

in float gs_progress;
uniform float uOffsetRatio;
void main()
{
    vec3 headColor = vec3(0.7, 0.3, 0.3);
    vec3 tailColor = vec3(0.3, 0.7, 0.3);
    vec3 lineColor = mix(headColor, tailColor, gs_progress);
    
    if (uOffsetRatio < 1.01f) {
        FragColor = vec4(lineColor, 0.95);
    } else {
        FragColor = vec4(0.2, 0.2, 0.7, 0.95);
    }
}

