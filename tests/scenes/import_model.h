#pragma once

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "tests/scenes/import_mesh.h"
#include <string>
#include <vector>

// Model class for loading and managing 3D models using Assimp
class import_model {
public:
  explicit import_model(const std::string &_path);
  ~import_model() = default;

  // Disable copy constructor and copy assignment
  import_model(const import_model &) = delete;
  import_model &operator=(const import_model &) = delete;

  // Enable move constructor and move assignment
  import_model(import_model &&other) noexcept = default;
  import_model &operator=(import_model &&other) noexcept = default;

  // Draw all meshes in the model
  void draw(shader *_shader);

  // Public data members
  std::vector<import_mesh> m_meshes;
  std::string m_directory;

private:
  // Load model from file path
  void load_model(const std::string &_path);

  // Extract directory from file path
  static std::string extract_directory(const std::string &_path);

  // Process scene node recursively
  void process_node(aiNode *_node, const aiScene *_scene);
  void process_node_impl(aiNode *_node, const aiScene *_scene, int _depth);

  // Process a single mesh from Assimp
  import_mesh process_mesh(aiMesh *_mesh, const aiScene *_scene);

  // Load textures from material
  void load_material_textures(aiMaterial *_material, aiTextureType _type,
                              std::vector<import_texture> &_textures);

  // Constants
  static constexpr int MAX_RECURSION_DEPTH = 100;
};
