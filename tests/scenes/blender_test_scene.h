#pragma once

#include "renderable_scene_base.h"

class blender_test_scene : public renderable_scene_base {
public:
  blender_test_scene();
  virtual ~blender_test_scene() = default;

  void init(GLFWwindow *window) override;
  void render() override;
  void render_ui() override;

protected:
  glm::vec3 m_front_object_position = {0.0f, 0.0f, 0.0f};
  glm::vec3 m_front_object_color = {1.0f, 0.0f, 0.0f};
  glm::vec3 m_back_object_position = {0.0f, 0.0f, -1.0f};
  glm::vec3 m_back_object_color = {0.0f, 1.0f, 0.0f};
  float m_transparency = 0.6f;
  bool m_enable_blend = false;
};