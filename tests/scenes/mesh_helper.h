#pragma once

#include "basic/vertex_array_object.h"
#include <vector>

// Helper struct to encapsulate mesh data
struct mesh_data {
  const void *vertices;
  size_t vertex_size;
  const void *indices;
  size_t index_count;
  std::vector<vertex_attribute> attributes;
};

// Helper class to manage VAO/VBO/EBO setup
class mesh_helper {
public:
  mesh_helper();
  ~mesh_helper();

  // Disable copy constructor and copy assignment
  mesh_helper(const mesh_helper &) = delete;
  mesh_helper &operator=(const mesh_helper &) = delete;

  // Enable move constructor and move assignment
  mesh_helper(mesh_helper &&other) noexcept;
  mesh_helper &operator=(mesh_helper &&other) noexcept;

  // Setup mesh with vertex and index data
  void setup_mesh(const mesh_data &data);

  // Bind mesh for rendering
  void bind() const;

  // Get index count for drawing
  size_t get_index_count() const { return m_index_count; }

  // Draw the mesh
  void draw() const;

private:
  friend struct import_model_scene;
  vertex_array_object *m_VAO = nullptr;
  vertex_buffer_object *m_VBO = nullptr;
  index_buffer_object *m_EBO = nullptr;
  size_t m_index_count = 0;
};
