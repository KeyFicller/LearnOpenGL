#pragma once

#include "renderable_scene_base.h"
#include <glm/glm.hpp>

class stencil_test_scene : public renderable_scene_base {
public:
  stencil_test_scene();
  virtual ~stencil_test_scene() = default;

public:
  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  glm::vec3 m_light_color = glm::vec3{1.0f, 1.0f, 1.0f};
  glm::vec3 m_object_color = glm::vec3{0.8f, 0.2f, 0.0f};
  glm::vec3 m_light_position = glm::vec3{1.0f, 1.0f, 0.0f};

  float m_ambient_strength = 0.1f;
  float m_specular_strength = 0.5f;
  float m_shininess = 32.0f;

  bool m_stencil_test = false;
  int m_stencil_test_index = 0;
  float m_boundary_scale = 1.01f;
  glm::vec3 m_boundary_color = glm::vec3{0.0f, 0.0f, 1.0f};
  glm::vec3 m_object_position = glm::vec3{0.0f, 0.0f, 0.0f};
};