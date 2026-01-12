#pragma once

#include "scene_base.h"
#include "shader.h"
#include "vertex_array_object.h"
#include <glm/glm.hpp>

// Transform test scene
class transform_test_scene : public test_scene_base {
public:
  transform_test_scene();
  virtual ~transform_test_scene();

  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  glm::mat4 m_transform;
  vertex_array_object *m_VAO = nullptr;
  vertex_buffer_object *m_VBO = nullptr;
  shader *m_shader = nullptr;
  float m_rotation_speed;
  glm::vec3 m_rotation_axis;
};
