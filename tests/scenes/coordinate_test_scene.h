#pragma once

#include "glm/fwd.hpp"
#include "scene_base.h"
#include "basic/shader.h"
#include "basic/vertex_array_object.h"
#include <glm/glm.hpp>
#include <vector>

// Coordinate test scene
class coordinate_test_scene : public test_scene_base {
public:
  coordinate_test_scene();
  virtual ~coordinate_test_scene();

  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  vertex_array_object *m_VAO = nullptr;
  vertex_buffer_object *m_VBO = nullptr;
  index_buffer_object *m_EBO = nullptr;
  shader *m_shader = nullptr;

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