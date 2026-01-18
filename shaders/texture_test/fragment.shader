#version 330 core
out vec4 FragColor;

in vec4 vertexColor;
in vec2 texCoord;
uniform vec4 ourColor;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform float uMix;

void main()
{
    FragColor = mix(texture(texture1, texCoord), texture(texture2, texCoord), uMix);
}

