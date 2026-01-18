#pragma once

#include "tests/component/sub_scene.h"
#include <glm/glm.hpp>

// Forward declaration
class stencil_test_scene;

/**
 * @brief Sub-scene: Object Outline using stencil test
 */
class stencil_object_outline_sub_scene : public sub_scene<stencil_test_scene> {
public:
  stencil_object_outline_sub_scene(stencil_test_scene *parent);

  void render() override;
  void render_ui() override;

private:
  glm::vec3 m_object_position = glm::vec3{0.0f, 0.0f, 0.0f};
  float m_boundary_scale = 1.01f;
  glm::vec3 m_boundary_color = glm::vec3{0.0f, 0.0f, 1.0f};
};

/**
 * @brief Sub-scene: Mirror reflection using stencil test
 */
class stencil_mirror_sub_scene : public sub_scene<stencil_test_scene> {
public:
  stencil_mirror_sub_scene(stencil_test_scene *parent);

  void render() override;
  void render_ui() override;

private:
  glm::vec3 m_object_position = glm::vec3{0.0f, 0.0f, 0.0f};
  glm::vec3 m_mirror_position = glm::vec3{0.0f, 0.0f, -5.0f};
  glm::vec3 m_mirror_scale = glm::vec3{5.0f, 5.0f, 0.1f};
  float m_mirror_alpha = 0.3f;
};
