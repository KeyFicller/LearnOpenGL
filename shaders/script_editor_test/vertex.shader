#version 330 core
layout (location = 0) in vec3 aPos;
uniform vec2 uOffset;
uniform float uScale;
void main()
{
  gl_Position = vec4(aPos * uScale + vec3(uOffset, 0.0), 1.0);
}
