#version 330 core
out vec4 FragColor;

in vec4 gs_color;
in float gs_segment_progress;

void main()
{
    vec3 finalColor = gs_color.rgb;
    
    finalColor += vec3(0.1, 0.2, 0.1) * (1.0 - gs_segment_progress);
    
    FragColor = vec4(finalColor, 1.0);
}

