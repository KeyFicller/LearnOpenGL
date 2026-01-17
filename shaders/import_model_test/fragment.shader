#version 330 core
out vec4 FragColor;

// struct Light {
//     int type;
//     vec3 position;
//     vec3 ambient;
//     vec3 diffuse;
//     vec3 specular;
//     vec3 direction;
//     float constant;
//     float linear;
//     float quadratic;
//     float cutoff;
//     float outer_cutoff;
// };

// struct Material {
//     float shininess;
// };

// uniform Light uLight[4];
// uniform Material uMaterial;
// uniform vec3 uEyePosition;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D uTextureDiffuse0;

// vec3 calc_directional_light(Light light, vec3 normal, vec3 view_dir)
// {
//     vec3 light_dir = normalize(-light.direction);

//     float diff = max(dot(normal, light_dir), 0.0);

//     vec3 reflect_dir = reflect(-light_dir, normal);

//     float spec = pow(max(dot(view_dir, reflect_dir), 0.0), uMaterial.shininess);

//     vec3 ambient = light.ambient * uMaterial.ambient;
//     vec3 diffuse = light.diffuse * diff * uMaterial.diffuse;
//     vec3 specular = light.specular * spec * uMaterial.specular;

//     return ambient + diffuse + specular;
// }

// vec3 calc_point_light(Light light, vec3 normal, vec3 view_dir)
// {
//     vec3 light_dir = normalize(light.position - FragPos);

//     float diff = max(dot(normal, light_dir), 0.0);

//     vec3 reflect_dir = reflect(-light_dir, normal);

//     float spec = pow(max(dot(view_dir, reflect_dir), 0.0), uMaterial.shininess);

//     float distance = length(light.position - FragPos);
//     float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

//     vec3 ambient = light.ambient * uMaterial.ambient;
//     vec3 diffuse = light.diffuse * diff * uMaterial.diffuse;
//     vec3 specular = light.specular * spec * uMaterial.specular;

//     return (ambient + diffuse + specular) * attenuation;
// }

// vec3 calc_spot_light(Light light, vec3 normal, vec3 view_dir)
// {
//     vec3 light_dir = normalize(light.position - FragPos);

//     float diff = max(dot(normal, light_dir), 0.0);

//     vec3 reflect_dir = reflect(-light_dir, normal);

//     float spec = pow(max(dot(view_dir, reflect_dir), 0.0), uMaterial.shininess);

//     float distance = length(light.position - FragPos);
//     float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

//     float theta = dot(light_dir, normalize(-light.direction));

//     float epsilon = light.cutoff - light.outer_cutoff;

//     float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.0, 1.0);

//     vec3 ambient = light.ambient * uMaterial.ambient;
//     vec3 diffuse = light.diffuse * diff * uMaterial.diffuse;
//     vec3 specular = light.specular * spec * uMaterial.specular;

//     return (ambient + diffuse + specular) * intensity * attenuation;
// }

void main()
{
    // Use texture if available, otherwise use a default color
    vec4 texColor = texture(uTextureDiffuse0, TexCoords);
    if (texColor.a < 0.1) {
        // If texture is not loaded or transparent, use a default color
        FragColor = vec4(0.8, 0.8, 0.8, 1.0);
    } else {
        FragColor = vec4(texColor.rgb, 1.0);
    }
}

