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
  m_mesh_manager.setup_mesh(data);
}

import_mesh::import_mesh(import_mesh &&other) noexcept
    : vertices(std::move(other.vertices)), indices(std::move(other.indices)),
      textures(std::move(other.textures)),
      m_mesh_manager(std::move(other.m_mesh_manager)) {}

import_mesh &import_mesh::operator=(import_mesh &&other) noexcept {
  if (this != &other) {
    vertices = std::move(other.vertices);
    indices = std::move(other.indices);
    textures = std::move(other.textures);
    m_mesh_manager = std::move(other.m_mesh_manager);
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
      std::string uniform_name = textures[i].type + std::to_string(i);
      _shader->set_uniform(uniform_name.c_str(), static_cast<int>(i));
    }
  }

  // If no textures, set a default texture unit to avoid shader errors
  if (textures.empty()) {
    _shader->set_uniform("uTextureDiffuse0", 0);
  }

  // Draw the mesh
  m_mesh_manager.draw();
}

