#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
uniform vec3 uEyePosition;
uniform samplerCube uCubeMap;
uniform float uRefractRatio;
uniform int uTestMode;

void main()
{
    vec3 I = normalize(FragPos - uEyePosition);
    vec3 R = uTestMode == 0 ? refract(I, normalize(Normal), uRefractRatio) : reflect(I, normalize(Normal));
    FragColor = vec4(texture(uCubeMap, R).rgb, 1.0);
}

