#pragma once

#include "basic/shader.h"
#include "basic/texture.h"
#include "tests/component/mesh_manager.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

// Vertex structure for imported models
struct import_vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texture_coords;
};

// Texture structure for imported models
struct import_texture {
  std::shared_ptr<texture_2d> texture;
  std::string type; // e.g., "uTextureDiffuse", "uTextureSpecular"
};

// Mesh class representing a single mesh in an imported model
class import_mesh {
public:
  import_mesh(const std::vector<import_vertex> &_vertices,
              const std::vector<unsigned int> &_indices,
              const std::vector<import_texture> &_textures);

  // Disable copy constructor and copy assignment (OpenGL resources)
  import_mesh(const import_mesh &) = delete;
  import_mesh &operator=(const import_mesh &) = delete;

  // Enable move constructor and move assignment
  import_mesh(import_mesh &&other) noexcept;
  import_mesh &operator=(import_mesh &&other) noexcept;

  // Draw the mesh with the given shader
  void draw(shader *_shader);

  // Public data members
  std::vector<import_vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<import_texture> textures;

private:
  mesh_manager m_mesh_manager;
};
