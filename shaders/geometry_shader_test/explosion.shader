#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
  vec3 position;
} gs_in[];

uniform float uShapeFactor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

vec4 explode(vec4 position, vec3 normal) {
  float magnitude = uShapeFactor;
  return position + vec4(normal * magnitude, 0.0);
}

vec3 getNormal()
{
  vec3 pos0 = gs_in[0].position;
  vec3 pos1 = gs_in[1].position;
  vec3 pos2 = gs_in[2].position;
  return normalize(cross(pos2 - pos1, pos0 - pos1));
}

void main()
{
  mat4 mvp = projection * view * model;
  vec3 normal = getNormal();
  vec4 pos = vec4(gs_in[0].position, 1.0);
  gl_Position = mvp * explode(pos, normal);
  EmitVertex();
  pos = vec4(gs_in[1].position, 1.0);
  gl_Position = mvp * explode(pos, normal);
  EmitVertex();
  pos = vec4(gs_in[2].position, 1.0);
  gl_Position = mvp * explode(pos, normal);
  EmitVertex();
  EndPrimitive();
}