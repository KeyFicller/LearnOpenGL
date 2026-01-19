#version 330 core
out vec4 FragColor;

in vec3 textureDir;
uniform samplerCube uCubeMap;

void main()
{
    FragColor = texture(uCubeMap, textureDir);
}

