#version 330 core
out vec4 FragColor;

uniform float uOffsetRatio;
void main()
{
    if (uOffsetRatio < 1.01f) {
        FragColor = vec4(0.8, 0.4, 0.4, 1.0);
    } else {
        FragColor = vec4(0.2, 0.2, 0.7, 1.0);
    }
}

