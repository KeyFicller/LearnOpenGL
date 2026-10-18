#version 330 core
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

// NDC position inside the board quad, interpolated for the fragment shader.
// Every line on the board is drawn from this position rather than from
// geometry, so the edges can be antialiased and the wood can have grain.
out vec2 gs_pos;

void main() {
  vec3 origin = gl_in[0].gl_Position.xyz;
  // Kept behind the pieces and hints, which sit at more negative z.
  float z = origin.z + 0.1;

  gs_pos = vec2(-1.0, -1.0);
  gl_Position = vec4(origin.xy + gs_pos, z, 1.0);
  EmitVertex();
  gs_pos = vec2(1.0, -1.0);
  gl_Position = vec4(origin.xy + gs_pos, z, 1.0);
  EmitVertex();
  gs_pos = vec2(-1.0, 1.0);
  gl_Position = vec4(origin.xy + gs_pos, z, 1.0);
  EmitVertex();
  gs_pos = vec2(1.0, 1.0);
  gl_Position = vec4(origin.xy + gs_pos, z, 1.0);
  EmitVertex();
  EndPrimitive();
}
