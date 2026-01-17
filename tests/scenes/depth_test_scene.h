#pragma once

#include "renderable_scene_base.h"
#include <glm/glm.hpp>

class depth_test_scene : public renderable_scene_base {
public:
  depth_test_scene();
  virtual ~depth_test_scene() = default;

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

  glm::vec3 m_object_position = glm::vec3{0.0f, 0.0f, 0.0f};
  glm::vec3 m_object_position2 = glm::vec3{0.2f, 0.2f, 0.2f};
  int m_depth_test_index = 0;

  bool m_depth_visualization = false;
};