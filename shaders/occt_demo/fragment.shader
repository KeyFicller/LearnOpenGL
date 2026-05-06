#version 330 core
in vec3 Normal;
in vec3 FragPos;
out vec4 FragColor;

uniform vec3 uLightDir;
uniform vec3 uViewPos;
uniform vec3 uObjectColor;

void main() {
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(-uLightDir);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 viewDir = normalize(uViewPos - FragPos);
  vec3 halfway = normalize(lightDir + viewDir);
  float spec = pow(max(dot(norm, halfway), 0.0), 64.0);
  vec3 rgb = uObjectColor * (0.12 + 0.55 * diff + 0.25 * spec);
  FragColor = vec4(rgb, 1.0);
}
