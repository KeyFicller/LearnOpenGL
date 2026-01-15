#version 330 core
out vec4 FragColor;

struct Light {
    int type;
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform Light uLight;
uniform Material uMaterial;
uniform vec3 uEyePosition;

in vec3 FragPos;
in vec3 Normal;

void main()
{
    vec3 ambient = uLight.ambient * uMaterial.ambient;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLight.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLight.diffuse;

    vec3 viewDir = normalize(uEyePosition - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial.shininess);
    vec3 specular = uLight.specular * spec * uMaterial.specular;

    FragColor = vec4((ambient + diffuse + specular), 1.0);
}

