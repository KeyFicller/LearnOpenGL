#pragma once

#include "renderable_scene_base.h"
#include "tests/component/mesh_manager.h"
#include <string>

class tessellation_shader_scene : public renderable_scene_base {
public:
  tessellation_shader_scene();
  ~tessellation_shader_scene() override;

public:
  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  mesh_manager m_patch_mesh;
  shader *m_shader = nullptr;
  float m_outer_level = 4.0f;
  float m_inner_level = 4.0f;
  float m_amplitude = 0.15f;
  bool m_supported = true;
  std::string m_error_message;
};
