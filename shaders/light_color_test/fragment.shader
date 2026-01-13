#version 330 core
out vec4 FragColor;

uniform vec3 uLightColor;
uniform vec3 uObjectColor;

uniform float uAmbientStrength;

in vec3 FragPos;
in vec3 Normal;
uniform vec3 uLightPosition;
uniform vec3 uEyePosition;
uniform float uSpecularStrength;
uniform float uShininess;

void main()
{
    vec3 ambient = uAmbientStrength * uLightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPosition - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;

    vec3 viewDir = normalize(uEyePosition - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uShininess);
    vec3 specular = uSpecularStrength * spec * uLightColor;

    FragColor = vec4((ambient + diffuse + specular) * uObjectColor, 1.0);
}

