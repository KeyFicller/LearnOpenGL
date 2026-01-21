#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec2 uTranslations[100];

void main()
{
    gl_Position = projection * view * model * vec4((aPos + vec3(uTranslations[gl_InstanceID], 0.0)), 1.0);
}

