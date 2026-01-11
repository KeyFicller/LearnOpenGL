#pragma once

#include "glm/fwd.hpp"
#include "scene_base.h"
#include "shader.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>

// Coordinate test scene
class coordinate_test_scene : public test_scene_base {
public:
  coordinate_test_scene();
  virtual ~coordinate_test_scene();

  void init() override;
  void render() override;
  void render_ui() override;

private:
  GLuint m_VAO;
  GLuint m_VBO;
  GLuint m_EBO;
  shader *m_shader;

  float m_rotation_angle = 45.0f;
  glm::vec3 m_rotation_axis = glm::vec3(0.0f, 0.0f, 1.0f);
  glm::vec3 m_camera_position = glm::vec3(0.0f, 0.0f, 3.0f);
  float m_fov = 45.0f;
  float m_height = 600.0f;
  float m_width = 800.0f;

  int m_editing_position_index = 0;
  std::vector<glm::vec3> m_positions = {
      {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}};
};