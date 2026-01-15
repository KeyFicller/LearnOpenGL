#version 330 core
out vec4 FragColor;

struct Light {
    int type;
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 direction;
    float constant;
    float linear;
    float quadratic;
    float cutoff;
    float outer_cutoff;
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
    vec3 lightDir;
    if (uLight.type == 0)
    {
        lightDir = normalize(-uLight.direction);
    }
    else if (uLight.type == 1 || uLight.type == 2)
    {
        lightDir = normalize(uLight.position - FragPos);
    }
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLight.diffuse * vec3(texture(uMaterial.diffuse, TexCoord));

    vec3 viewDir = normalize(uEyePosition - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial.shininess);
    vec3 specular = uLight.specular * spec * vec3(texture(uMaterial.specular, TexCoord));

    // Calculate distance attenuation for point and spot lights
    float attenuation = 1.0;
    if (uLight.type == 1 || uLight.type == 2)
    {
        float distance = length(uLight.position - FragPos);
        attenuation = 1.0 / (uLight.constant + uLight.linear * distance + uLight.quadratic * distance * distance);
        // Apply attenuation to diffuse and specular (ambient should not be attenuated)
        diffuse *= attenuation;
        specular *= attenuation;
    }

    // Apply spot light angle attenuation
    if (uLight.type == 2)
    {
        float theta = dot(lightDir, normalize(-uLight.direction));
        float epsilon = uLight.cutoff - uLight.outer_cutoff;
        float intensity = clamp((theta - uLight.outer_cutoff) / epsilon, 0.0, 1.0);
        diffuse *= intensity;
        specular *= intensity;
    }

    FragColor = vec4((ambient + diffuse + specular), 1.0);
}

