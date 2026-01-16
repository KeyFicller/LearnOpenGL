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
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform Light uLight[4];
uniform Material uMaterial;
uniform vec3 uEyePosition;

in vec3 FragPos;
in vec3 Normal;

vec3 calc_directional_light(Light light, vec3 normal, vec3 view_dir)
{
    vec3 light_dir = normalize(-light.direction);

    float diff = max(dot(normal, light_dir), 0.0);

    vec3 reflect_dir = reflect(-light_dir, normal);

    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), uMaterial.shininess);

    vec3 ambient = light.ambient * uMaterial.ambient;
    vec3 diffuse = light.diffuse * diff * uMaterial.diffuse;
    vec3 specular = light.specular * spec * uMaterial.specular;

    return ambient + diffuse + specular;
}

vec3 calc_point_light(Light light, vec3 normal, vec3 view_dir)
{
    vec3 light_dir = normalize(light.position - FragPos);

    float diff = max(dot(normal, light_dir), 0.0);

    vec3 reflect_dir = reflect(-light_dir, normal);

    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), uMaterial.shininess);

    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    vec3 ambient = light.ambient * uMaterial.ambient;
    vec3 diffuse = light.diffuse * diff * uMaterial.diffuse;
    vec3 specular = light.specular * spec * uMaterial.specular;

    return (ambient + diffuse + specular) * attenuation;
}

vec3 calc_spot_light(Light light, vec3 normal, vec3 view_dir)
{
    vec3 light_dir = normalize(light.position - FragPos);

    float diff = max(dot(normal, light_dir), 0.0);

    vec3 reflect_dir = reflect(-light_dir, normal);

    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), uMaterial.shininess);

    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    float theta = dot(light_dir, normalize(-light.direction));

    float epsilon = light.cutoff - light.outer_cutoff;

    float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.0, 1.0);

    vec3 ambient = light.ambient * uMaterial.ambient;
    vec3 diffuse = light.diffuse * diff * uMaterial.diffuse;
    vec3 specular = light.specular * spec * uMaterial.specular;

    return (ambient + diffuse + specular) * intensity * attenuation;
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 view_dir = normalize(uEyePosition - FragPos);

    vec3 result = vec3(0.0);
    for (int i = 0; i < 4; i++)
    {
        if (uLight[i].type == 0)
        {
            result += calc_directional_light(uLight[i], norm, view_dir);
        }
        else if (uLight[i].type == 1)
        {
            result += calc_point_light(uLight[i], norm, view_dir);
        }
        else if (uLight[i].type == 2)
        {
            result += calc_spot_light(uLight[i], norm, view_dir);
        }
    }

    FragColor = vec4(result, 1.0);
}

