#pragma once

#include "camera_scene_base.h"
#include "glm/ext/vector_float3.hpp"
#include "light.h"
#include "material.h"
#include "shader.h"
#include "vertex_array_object.h"

class light_type_scene : public camera_scene_base {
public:
  light_type_scene();
  virtual ~light_type_scene();

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

  light m_light;
  material m_material;
};