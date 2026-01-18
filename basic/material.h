#pragma once

#include "basic/shader.h"
#include "basic/texture.h"
#include "glm/glm.hpp"
#include "imgui.h"

/**
 * @brief Material with color properties
 */
struct material {
  glm::vec3 Ambient = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 Diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 Specular = glm::vec3(1.0f, 1.0f, 1.0f);
  float Shininess = 32.0f;
};

/**
 * @brief Material with texture properties
 */
struct texture_material {
  texture_2d *DiffuseTexture = nullptr;
  texture_2d *SpecularTexture = nullptr;
  float Shininess = 32.0f;

  ~texture_material() {
    delete DiffuseTexture;
    delete SpecularTexture;
  }
};

// -----------------------------------------------------------------------------
/**
 * @brief Set uniform values for a material
 * @param _shader The shader to set the uniform values for
 * @param _material The material to set the uniform values for
 * @param _name The name of the material
 */
void uniform(shader &_shader, const material &_material, std::string _name);

/**
 * @brief UI for a material
 * @param _material The material to display in the UI
 */
void ui(material &_material);

// -----------------------------------------------------------------------------
/**
 * @brief Set uniform values for a texture material
 * @param _shader The shader to set the uniform values for
 * @param _texture_material The texture material to set the uniform values for
 * @param _name The name of the texture material
 */
void uniform(shader &_shader, const texture_material &_texture_material,
             std::string _name);

/**
 * @brief UI for a texture material
 * @param _texture_material The texture material to display in the UI
 */
void ui(texture_material &_texture_material);