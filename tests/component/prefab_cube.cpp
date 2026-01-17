#include "tests/component/prefab_cube.h"

namespace prefab_cube {

// Cube vertices for position + normal format (24 vertices * 6 floats)
// Each face has 4 vertices with position (3) + normal (3)
static const float cube_vertices_position_normal[] = {
    // Back face (z = -0.5), normal: (0, 0, -1)
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, // 0: back bottom left
    0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,  // 1: back bottom right
    0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,   // 2: back top right
    -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,  // 3: back top left

    // Front face (z = 0.5), normal: (0, 0, 1)
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,  // 4: front bottom right
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, // 5: front bottom left
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,  // 6: front top left
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,   // 7: front top right

    // Left face (x = -0.5), normal: (-1, 0, 0)
    -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f,  // 8: left bottom front
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, // 9: left bottom back
    -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f,  // 10: left top back
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,   // 11: left top front

    // Right face (x = 0.5), normal: (1, 0, 0)
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, // 12: right bottom back
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,  // 13: right bottom front
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,   // 14: right top front
    0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,  // 15: right top back

    // Bottom face (y = -0.5), normal: (0, -1, 0)
    -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,  // 16: bottom front left
    0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,   // 17: bottom front right
    0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,  // 18: bottom back right
    -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, // 19: bottom back left

    // Top face (y = 0.5), normal: (0, 1, 0)
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, // 20: top back left
    0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  // 21: top back right
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,   // 22: top front right
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,  // 23: top front left
};

// Cube vertices for position + normal + texture format (24 vertices * 8 floats)
static const float cube_vertices_position_normal_tex[] = {
    // Back face (z = -0.5), normal: (0, 0, -1)
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // 0: back bottom left
    0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,  // 1: back bottom right
    0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,   // 2: back top right
    -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,  // 3: back top left

    // Front face (z = 0.5), normal: (0, 0, 1)
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // 4: front bottom right
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // 5: front bottom left
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,  // 6: front top left
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,   // 7: front top right

    // Left face (x = -0.5), normal: (-1, 0, 0)
    -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // 8: left bottom front
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // 9: left bottom back
    -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // 10: left top back
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,   // 11: left top front

    // Right face (x = 0.5), normal: (1, 0, 0)
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 12: right bottom back
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // 13: right bottom front
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // 14: right top front
    0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // 15: right top back

    // Bottom face (y = -0.5), normal: (0, -1, 0)
    -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // 16: bottom front left
    0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,  // 17: bottom front right
    0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // 18: bottom back right
    -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // 19: bottom back left

    // Top face (y = 0.5), normal: (0, 1, 0)
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // 20: top back left
    0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // 21: top back right
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,   // 22: top front right
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // 23: top front left
};

// Cube vertices for position only format (8 vertices * 3 floats)
static const float cube_vertices_position_only[] = {
    // Back face (z = -0.5)
    -0.5f, -0.5f, -0.5f, // 0: back bottom left
    0.5f, -0.5f, -0.5f,  // 1: back bottom right
    0.5f, 0.5f, -0.5f,   // 2: back top right
    -0.5f, 0.5f, -0.5f,  // 3: back top left
    // Front face (z = 0.5)
    -0.5f, -0.5f, 0.5f, // 4: front bottom left
    0.5f, -0.5f, 0.5f,  // 5: front bottom right
    0.5f, 0.5f, 0.5f,   // 6: front top right
    -0.5f, 0.5f, 0.5f,  // 7: front top left
};

// Cube indices (36 indices for 6 faces * 2 triangles * 3 vertices)
static const unsigned int cube_indices[] = {
    // Back face
    0, 1, 2, 2, 3, 0,
    // Front face
    4, 5, 6, 6, 7, 4,
    // Left face
    8, 9, 10, 10, 11, 8,
    // Right face
    12, 13, 14, 14, 15, 12,
    // Bottom face
    16, 17, 18, 18, 19, 16,
    // Top face
    20, 21, 22, 22, 23, 20,
};

// Simplified indices for position-only format (using 8 vertices)
static const unsigned int cube_indices_position_only[] = {
    // Back face
    0, 1, 2, 2, 3, 0,
    // Front face
    4, 5, 6, 6, 7, 4,
    // Left face
    7, 4, 0, 0, 3, 7,
    // Right face
    1, 5, 6, 6, 2, 1,
    // Bottom face
    4, 5, 1, 1, 0, 4,
    // Top face
    3, 2, 6, 6, 7, 3};

std::vector<float> get_vertices(vertex_format format) {
  switch (format) {
  case vertex_format::position_only: {
    const float *data = cube_vertices_position_only;
    return std::vector<float>(data, data + 8 * 3);
  }
  case vertex_format::position_normal: {
    const float *data = cube_vertices_position_normal;
    return std::vector<float>(data, data + 24 * 6);
  }
  case vertex_format::position_normal_tex: {
    const float *data = cube_vertices_position_normal_tex;
    return std::vector<float>(data, data + 24 * 8);
  }
  default:
    return std::vector<float>();
  }
}

std::vector<unsigned int> get_indices() {
  const unsigned int *data = cube_indices;
  return std::vector<unsigned int>(data, data + 36);
}

std::vector<vertex_attribute> get_attributes(vertex_format format) {
  switch (format) {
  case vertex_format::position_only:
    return {{3, GL_FLOAT, GL_FALSE}};
  case vertex_format::position_normal:
    return {{3, GL_FLOAT, GL_FALSE}, {3, GL_FLOAT, GL_FALSE}};
  case vertex_format::position_normal_tex:
    return {{3, GL_FLOAT, GL_FALSE}, {3, GL_FLOAT, GL_FALSE},
            {2, GL_FLOAT, GL_FALSE}};
  default:
    return {};
  }
}

cube_mesh_data::cube_mesh_data(vertex_format format) {
  vertices = get_vertices(format);
  indices = get_indices();
  
  // For position_only format, use the simplified indices
  if (format == vertex_format::position_only) {
    const unsigned int *idx_data = cube_indices_position_only;
    indices = std::vector<unsigned int>(idx_data, idx_data + 36);
  }

  mesh.vertices = vertices.data();
  mesh.vertex_size = sizeof(vertices[0]) * vertices.size();
  mesh.indices = indices.data();
  mesh.index_count = indices.size();
  mesh.attributes = get_attributes(format);
}

} // namespace prefab_cube

