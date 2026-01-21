#version 330 core

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;


in VS_OUT {
  vec3 position;
} gs_in[];

uniform float uShapeFactor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

vec3 getNormal()
{
  vec3 pos0 = gs_in[0].position;
  vec3 pos1 = gs_in[1].position;
  vec3 pos2 = gs_in[2].position;
  return normalize(cross(pos2 - pos1, pos0 - pos1));
}

void genLine(int index)
{
  mat4 mvp = projection * view * model;
  vec3 normal = getNormal();
  vec4 pos = vec4(gs_in[index].position, 1.0);
  gl_Position = mvp * (pos + vec4(normal * uShapeFactor, 0.0));
  EmitVertex();
  gl_Position = mvp * pos;
  EmitVertex();
  EndPrimitive();
}

void main()
{
  genLine(0);
  genLine(1);
  genLine(2);
}