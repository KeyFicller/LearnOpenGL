#pragma once

#include "basic/vertex_array.h"
#include <vector>

// Helper struct to encapsulate mesh data
struct mesh_data {
  const void *vertices;
  size_t vertex_size;
  const void *indices;
  size_t index_count;
  std::vector<vertex_attribute> attributes;

  mesh_data(const void *_vertices, size_t _vertex_size, const void *_indices,
            size_t _index_count, std::vector<vertex_attribute> _attributes)
      : vertices(_vertices), vertex_size(_vertex_size), indices(_indices),
        index_count(_index_count), attributes(_attributes) {}
  mesh_data(const void *_vertices, size_t _vertex_size, size_t _vertex_count,
            const std::vector<vertex_attribute> &_attributes)
      : vertices(_vertices), vertex_size(_vertex_size), indices(nullptr),
        index_count(_vertex_count), attributes(_attributes) {}
};

// Helper class to manage VAO/VBO/EBO setup
class mesh_manager {
public:
  mesh_manager();
  ~mesh_manager();

  // Disable copy constructor and copy assignment
  mesh_manager(const mesh_manager &) = delete;
  mesh_manager &operator=(const mesh_manager &) = delete;

  // Enable move constructor and move assignment
  mesh_manager(mesh_manager &&other) noexcept;
  mesh_manager &operator=(mesh_manager &&other) noexcept;

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
  vertex_array *m_VAO = nullptr;
  vertex_buffer *m_VBO = nullptr;
  index_buffer *m_EBO = nullptr;
  size_t m_index_count = 0;
};
