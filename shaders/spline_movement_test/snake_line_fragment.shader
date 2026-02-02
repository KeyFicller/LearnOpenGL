#version 330 core
out vec4 FragColor;

in float gs_progress;

void main()
{
    vec3 headColor = vec3(0.2, 0.9, 0.3);
    vec3 tailColor = vec3(0.1, 0.5, 0.15);
    vec3 lineColor = mix(headColor, tailColor, gs_progress);
    
    lineColor += vec3(0.05, 0.1, 0.05) * (1.0 - gs_progress);
    
    FragColor = vec4(lineColor, 0.95);
}

