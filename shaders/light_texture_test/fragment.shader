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
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

uniform Light uLight;
uniform Material uMaterial;
uniform vec3 uEyePosition;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

void main()
{
    vec3 ambient = uLight.ambient * vec3(texture(uMaterial.diffuse, TexCoord));

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLight.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLight.diffuse * vec3(texture(uMaterial.diffuse, TexCoord));

    vec3 viewDir = normalize(uEyePosition - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial.shininess);
    vec3 specular = uLight.specular * spec * vec3(texture(uMaterial.specular, TexCoord));

    FragColor = vec4((ambient + diffuse + specular), 1.0);
}

