#pragma once

#include "glm/glm.hpp"
#include "imgui.h"
#include "shader.h"

/**
 * @brief Light type
 */
enum class light_type : int {
  k_directional, // Light is coming from a direction
  k_point,       // Light is coming from a point
  k_spot,        // Light is coming from a spot
};

/**
 * @brief Light
 */
struct light {
  // Common properties
  glm::vec3 Ambient = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 Diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 Specular = glm::vec3(1.0f, 1.0f, 1.0f);

  // Type of light
  light_type Type = light_type::k_point;

  // Properties for directional and spot lights
  glm::vec3 Direction = glm::vec3(0.0f, 0.0f, -1.0f);

  // Properties for point and spot lights
  glm::vec3 Position = glm::vec3(1.0f, 1.0f, 0.0f);
  float Constant = 1.0f;
  float Linear = 0.09f;
  float Quadratic = 0.032f;

  // Properties for spot lights
  float Cutoff = 12.5f;      // Inner cutoff angle in degrees
  float OuterCutoff = 17.5f; // Outer cutoff angle in degrees
};

// -----------------------------------------------------------------------------
/**
 * @brief Set uniform values for a light
 * @param _shader The shader to set the uniform values for
 * @param _light The light to set the uniform values for
 * @param _name The name of the light
 * note: Light struct defines in shader like: struct Light {
 *   int type;
 *   vec3 ambient;
 *   vec3 diffuse;
 *   vec3 specular;
 *   vec3 position;
 *   vec3 direction;
 *   float constant;
 *   float linear;
 *   float quadratic;
 *   float cutoff;
 *   float outer_cutoff;
 * };
 */
void uniform(shader &_shader, const light &_light, std::string _name);

// -----------------------------------------------------------------------------
/**
 * @brief UI for a light
 * @param _light The light to display in the UI
 */
void ui(light &_light);