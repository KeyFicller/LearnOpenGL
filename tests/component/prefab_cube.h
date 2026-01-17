#pragma once

#include "tests/component/mesh_manager.h"
#include <vector>

// Prefab cube geometry generator
namespace prefab_cube {
// Cube vertex format types
enum class vertex_format {
  position_only,      // 3 floats: position (x, y, z)
  position_normal,    // 6 floats: position (x, y, z) + normal (nx, ny, nz)
  position_normal_tex // 8 floats: position + normal + texture coords (u, v)
};

// Get cube vertices as a vector of floats
std::vector<float> get_vertices(vertex_format format);

// Get cube indices as a vector of unsigned ints
std::vector<unsigned int> get_indices();

// Get vertex attributes for the given format
std::vector<vertex_attribute> get_attributes(vertex_format format);

// Structure to hold cube mesh data with vectors
struct cube_mesh_data {
  std::vector<float> vertices;
  std::vector<unsigned int> indices;
  mesh_data mesh;

  cube_mesh_data(vertex_format format);
};
} // namespace prefab_cube

