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

vec3 cal_directional_light(Light light, vec3 normal, vec3 view_dir)
{
    vec3 light_dir = normalize(-light.direction);
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), uMaterial.shininess);
    return light.ambient * uMaterial.ambient + light.diffuse * diff * uMaterial.diffuse + light.specular * spec * uMaterial.specular;
}

vec3 cal_point_light(Light light, vec3 normal, vec3 view_dir)
{
    vec3 light_dir = normalize(light.position - FragPos);
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), uMaterial.shininess);

    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    return (light.ambient * uMaterial.ambient + light.diffuse * diff * uMaterial.diffuse + light.specular * spec * uMaterial.specular) * attenuation;
}

vec3 cal_spot_light(Light light, vec3 normal, vec3 view_dir)
{
    vec3 light_dir = normalize(light.position - FragPos);
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), uMaterial.shininess);

    float theta = dot(light_dir, normalize(-light.direction));
    float epsilon = light.cutoff - light.outer_cutoff;
    float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.0, 1.0);

    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    return (light.ambient * uMaterial.ambient + light.diffuse * diff * uMaterial.diffuse + light.specular * spec * uMaterial.specular) * attenuation * intensity;
}

void main()
{
    vec3 result = vec3(0.0);
    for (int i = 0; i < 4; i++)
    {
        if (uLight[i].type == 0)
        {
            result += cal_directional_light(uLight[i], Normal, normalize(uEyePosition - FragPos));
        }
        else if (uLight[i].type == 1)
        {
            result += cal_point_light(uLight[i], Normal, normalize(uEyePosition - FragPos));
        }
        else if (uLight[i].type == 2)
        {
            result += cal_spot_light(uLight[i], Normal, normalize(uEyePosition - FragPos));
        }
    }
    FragColor = vec4(result, 1.0);
}

