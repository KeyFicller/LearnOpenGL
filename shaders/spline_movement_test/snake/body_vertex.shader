#version 330 core
layout(location = 0) in vec3 aPos;

uniform int uTotalPoints;

out float vs_progress;

void main()
{
    vs_progress = float(gl_VertexID) / float(max(uTotalPoints - 1, 1));
    
    gl_Position = vec4(aPos, 1.0);
}

