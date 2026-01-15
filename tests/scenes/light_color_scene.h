#pragma once

#include "camera_scene_base.h"
#include "glm/ext/vector_float3.hpp"
#include "shader.h"
#include "vertex_array_object.h"

class light_color_scene : public camera_scene_base {
public:
  light_color_scene();
  virtual ~light_color_scene();

public:
  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  vertex_array_object *m_VAO = nullptr;
  vertex_buffer_object *m_VBO = nullptr;
  index_buffer_object *m_EBO = nullptr;
  shader *m_shader = nullptr;
  shader *m_light_shader = nullptr;
  glm::vec3 m_light_color = glm::vec3{1.0f, 1.0f, 1.0f};
  glm::vec3 m_object_color = glm::vec3{0.8f, 0.2f, 0.0f};
  glm::vec3 m_light_position = glm::vec3{1.0f, 1.0f, 0.0f};

  float m_ambient_strength = 0.1f;
  float m_specular_strength = 0.5f;
  float m_shininess = 32.0f;
};