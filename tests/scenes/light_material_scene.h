#pragma once

#include "camera.h"
#include "glm/ext/vector_float3.hpp"
#include "light.h"
#include "material.h"
#include "scene_base.h"
#include "shader.h"
#include "test_component/camera_controller.h"
#include "vertex_array_object.h"

class light_material_scene : public test_scene_base {
public:
  light_material_scene();
  virtual ~light_material_scene();

public:
  void init(GLFWwindow *_window) override;
  void update(float _delta_time) override;
  void render() override;
  void render_ui() override;
  bool on_mouse_moved(double _xpos, double _ypos) override;
  bool on_mouse_scroll(double _xoffset, double _yoffset) override;

private:
  camera m_camera;
  camera_controller *m_camera_controller = nullptr;
  vertex_array_object *m_VAO = nullptr;
  vertex_buffer_object *m_VBO = nullptr;
  index_buffer_object *m_EBO = nullptr;
  shader *m_shader = nullptr;
  shader *m_light_shader = nullptr;
  bool m_camera_controller_enabled = false;

  light m_light;
  material m_material;
};