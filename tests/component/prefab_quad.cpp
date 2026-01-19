#include "prefab_quad.h"

namespace prefab_quad {
// Quad vertices for position + normal format (4 vertices * 6 floats)
static const float quad_vertices_position_normal[] = {
    // Bottom left
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f, // 0: bottom left
    // Bottom right
    0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f, // 1: bottom right
    // Top left
    -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, -1.0f, // 2: top left
    // Top right
    0.5f, 0.5f, 0.0f, 0.0f, 0.0f, -1.0f, // 3: top right
};

static const float quad_vertices_position_only[] = {
    // Bottom left
    -0.5f, -0.5f, 0.0f, // 0: bottom left
    // Bottom right
    0.5f, -0.5f, 0.0f, // 1: bottom right
    // Top left
    -0.5f, 0.5f, 0.0f, // 2: top left
    // Top right
    0.5f, 0.5f, 0.0f, // 3: top right
};

// Quad vertices for position + normal + texture format (4 vertices * 8 floats)
static const float quad_vertices_position_normal_tex[] = {
    // Bottom left
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // 0: bottom left
    // Bottom right
    0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // 1: bottom right
    // Top left
    -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, // 2: top left
    // Top right
    0.5f, 0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // 3: top right
};

// Quad indices for position + normal format (2 triangles * 3 indices)
// CCW winding order (OpenGL default): face towards -Z direction
// Vertex layout: 0(bottom-left), 1(bottom-right), 2(top-left), 3(top-right)
static const unsigned int quad_indices_position_normal[] = {
    0, 2, 1, // First triangle: bottom-left -> top-left -> bottom-right (CCW)
    2, 3, 1, // Second triangle: top-left -> top-right -> bottom-right (CCW)
};

std::vector<float> get_vertices(vertex_format format) {
  switch (format) {
  case vertex_format::position_only: {
    return std::vector<float>(quad_vertices_position_only,
                              quad_vertices_position_only + 4 * 3);
  }
  case vertex_format::position_normal: {
    return std::vector<float>(quad_vertices_position_normal,
                              quad_vertices_position_normal + 4 * 6);
  }
  case vertex_format::position_normal_tex: {
    return std::vector<float>(quad_vertices_position_normal_tex,
                              quad_vertices_position_normal_tex + 4 * 8);
  }
  default:
    return std::vector<float>();
  }
}

std::vector<unsigned int> get_indices() {
  return std::vector<unsigned int>(quad_indices_position_normal,
                                   quad_indices_position_normal + 6);
}

std::vector<vertex_attribute> get_attributes(vertex_format format) {
  switch (format) {
  case vertex_format::position_only: {
    return std::vector<vertex_attribute>({{3, GL_FLOAT, false}});
  }
  case vertex_format::position_normal: {
    return std::vector<vertex_attribute>(
        {{3, GL_FLOAT, false}, {3, GL_FLOAT, false}});
  }
  case vertex_format::position_normal_tex: {
    return std::vector<vertex_attribute>(
        {{3, GL_FLOAT, false}, {3, GL_FLOAT, false}, {2, GL_FLOAT, false}});
  }
  default:
    return std::vector<vertex_attribute>();
  }
}

quad_mesh_data::quad_mesh_data(vertex_format format) {
  vertices = get_vertices(format);
  indices = get_indices();
  mesh = new mesh_data(vertices.data(), sizeof(vertices[0]) * vertices.size(),
                       indices.data(), indices.size(), get_attributes(format));
}

quad_mesh_data::~quad_mesh_data() { delete mesh; }

} // namespace prefab_quad