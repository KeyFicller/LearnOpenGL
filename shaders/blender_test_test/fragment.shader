#version 330 core
out vec4 FragColor;

uniform vec3 uObjectColor;
uniform float uTransparency;
void main()
{
    FragColor = vec4(uObjectColor, uTransparency);
}

