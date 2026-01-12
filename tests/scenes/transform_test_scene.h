#pragma once

#include "scene_base.h"
#include "shader.h"
#include <glad/gl.h>
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
  GLuint m_VAO;
  GLuint m_VBO;
  shader *m_shader;
  float m_rotation_speed;
  glm::vec3 m_rotation_axis;
};
