#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4 model;

layout (std140) uniform uCameraMatrics {
    mat4 view;
    mat4 projection;
};

uniform int uSubSenceIndex;
uniform float uPointSize;
uniform int uIncrementPointSize;

out vec3 Normal;
out vec3 FragPos;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    Normal = mat3(transpose(inverse(model))) * aNormal;
    FragPos = vec3(model * vec4(aPos, 1.0));

    if (uSubSenceIndex == 0) {
    float final_size = uPointSize;
    if (uIncrementPointSize != 0) {
        float multiplier = max(1.0, float(gl_VertexID) + 1.0);
        final_size = uPointSize * multiplier;
    }
    gl_PointSize = final_size;
    }
}

