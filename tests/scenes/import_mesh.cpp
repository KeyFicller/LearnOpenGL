#include "import_mesh.h"
#include "glad/gl.h"

import_mesh::import_mesh(const std::vector<import_vertex> &_vertices,
                         const std::vector<unsigned int> &_indices,
                         const std::vector<import_texture> &_textures)
    : vertices(_vertices), indices(_indices), textures(_textures) {
  // Setup mesh data for OpenGL
  mesh_data data;
  data.vertices = vertices.data();
  data.vertex_size = sizeof(vertices[0]);
  data.indices = indices.data();
  data.index_count = indices.size();
  // Vertex attributes: position (3), normal (3), texture_coords (2)
  data.attributes = {{3, GL_FLOAT, GL_FALSE},
                     {3, GL_FLOAT, GL_FALSE},
                     {2, GL_FLOAT, GL_FALSE}};
  m_mesh_helper.setup_mesh(data);
}

import_mesh::import_mesh(import_mesh &&other) noexcept
    : vertices(std::move(other.vertices)), indices(std::move(other.indices)),
      textures(std::move(other.textures)),
      m_mesh_helper(std::move(other.m_mesh_helper)) {}

import_mesh &import_mesh::operator=(import_mesh &&other) noexcept {
  if (this != &other) {
    vertices = std::move(other.vertices);
    indices = std::move(other.indices);
    textures = std::move(other.textures);
    m_mesh_helper = std::move(other.m_mesh_helper);
  }
  return *this;
}

void import_mesh::draw(shader *_shader) {
  if (!_shader) {
    return;
  }

  _shader->use();

  // Bind all textures
  for (unsigned int i = 0; i < textures.size(); i++) {
    if (textures[i].texture) {
      textures[i].texture->bind(i);
      int texture_slot = static_cast<int>(i);
      _shader->set_uniform<int>((textures[i].type + std::to_string(i)).c_str(),
                                 texture_slot);
    }
  }

  // Draw the mesh
  m_mesh_helper.draw();
}

