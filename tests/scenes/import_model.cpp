#include "import_model.h"
#include "basic/shader.h"
#include "basic/texture.h"
#include <iostream>

import_model::import_model(const std::string &_path) { load_model(_path); }

void import_model::load_model(const std::string &_path) {
  Assimp::Importer importer;
  const aiScene *scene =
      importer.ReadFile(_path, aiProcess_Triangulate | aiProcess_FlipUVs);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    throw std::runtime_error("ERROR::ASSIMP:: " +
                             std::string(importer.GetErrorString()));
  }

  // Extract directory for texture loading
  m_directory = extract_directory(_path);

  // Process the root node and all its children
  process_node(scene->mRootNode, scene);
}

std::string import_model::extract_directory(const std::string &_path) {
  // Try to find last directory separator
  size_t last_slash = _path.find_last_of('/');
  if (last_slash != std::string::npos) {
    return _path.substr(0, last_slash);
  }

  // Try Windows path separator
  size_t last_backslash = _path.find_last_of('\\');
  if (last_backslash != std::string::npos) {
    return _path.substr(0, last_backslash);
  }

  // No directory separator found, use current directory
  return ".";
}

void import_model::process_node(aiNode *_node, const aiScene *_scene) {
  process_node_impl(_node, _scene, 0);
}

void import_model::process_node_impl(aiNode *_node, const aiScene *_scene,
                                     int _depth) {
  // Safety check to prevent infinite recursion
  if (_depth > MAX_RECURSION_DEPTH) {
    std::cerr << "Warning: Maximum recursion depth reached in process_node"
              << std::endl;
    return;
  }

  if (!_node) {
    return;
  }

  // Process all meshes of this node
  for (unsigned int i = 0; i < _node->mNumMeshes; i++) {
    unsigned int mesh_index = _node->mMeshes[i];
    if (mesh_index < _scene->mNumMeshes) {
      aiMesh *mesh = _scene->mMeshes[mesh_index];
      m_meshes.push_back(std::move(process_mesh(mesh, _scene)));
    }
  }

  // Process all children recursively
  for (unsigned int i = 0; i < _node->mNumChildren; i++) {
    if (_node->mChildren[i]) {
      process_node_impl(_node->mChildren[i], _scene, _depth + 1);
    }
  }
}

import_mesh import_model::process_mesh(aiMesh *_mesh, const aiScene *_scene) {
  std::vector<import_vertex> vertices;
  std::vector<unsigned int> indices;
  std::vector<import_texture> textures;

  // Process vertices
  for (unsigned int i = 0; i < _mesh->mNumVertices; i++) {
    import_vertex vertex;

    // Position
    vertex.position = glm::vec3(_mesh->mVertices[i].x, _mesh->mVertices[i].y,
                                _mesh->mVertices[i].z);

    // Normal
    if (_mesh->mNormals) {
      vertex.normal = glm::vec3(_mesh->mNormals[i].x, _mesh->mNormals[i].y,
                                _mesh->mNormals[i].z);
    } else {
      vertex.normal = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    // Texture coordinates
    if (_mesh->mTextureCoords[0]) {
      vertex.texture_coords = glm::vec2(_mesh->mTextureCoords[0][i].x,
                                        _mesh->mTextureCoords[0][i].y);
    } else {
      vertex.texture_coords = glm::vec2(0.0f, 0.0f);
    }

    vertices.push_back(vertex);
  }

  // Process indices
  for (unsigned int i = 0; i < _mesh->mNumFaces; i++) {
    aiFace face = _mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++) {
      indices.push_back(face.mIndices[j]);
    }
  }

  // Process material and textures
  if (_mesh->mMaterialIndex >= 0) {
    aiMaterial *material = _scene->mMaterials[_mesh->mMaterialIndex];
    load_material_textures(material, aiTextureType_DIFFUSE, textures);
    load_material_textures(material, aiTextureType_SPECULAR, textures);
  }

  return import_mesh(vertices, indices, textures);
}

void import_model::load_material_textures(
    aiMaterial *_material, aiTextureType _type,
    std::vector<import_texture> &_textures) {
  unsigned int texture_count = _material->GetTextureCount(_type);

  for (unsigned int i = 0; i < texture_count; i++) {
    aiString str;
    if (_material->GetTexture(_type, i, &str) != AI_SUCCESS) {
      continue;
    }

    // Construct full texture path
    std::string texture_path = m_directory + "/" + std::string(str.C_Str());

    // Determine texture type name for shader
    std::string type_name = (_type == aiTextureType_DIFFUSE)
                                ? "uTextureDiffuse"
                                : "uTextureSpecular";

    // Try to load texture
    try {
      _textures.push_back(import_texture{
          std::make_shared<texture_2d>(texture_path.c_str()), type_name});
    } catch (const std::exception &e) {
      std::cerr << "Warning: Failed to load texture: " << texture_path << " - "
                << e.what() << std::endl;
      // Continue loading other textures instead of crashing
    }
  }
}

void import_model::draw(shader *_shader) {
  if (!_shader) {
    return;
  }

  _shader->use();
  for (auto &mesh : m_meshes) {
    mesh.draw(_shader);
  }
}
