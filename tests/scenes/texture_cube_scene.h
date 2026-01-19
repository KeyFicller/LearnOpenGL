#pragma once

#include "basic/shader.h"
#include "basic/texture.h"
#include "renderable_scene_base.h"
#include "tests/component/mesh_manager.h"

class texture_cube_scene : public renderable_scene_base {
public:
  texture_cube_scene();
  virtual ~texture_cube_scene();

  void init(GLFWwindow *window) override;
  void render() override;
  void render_ui() override;

protected:
  texture_cube *m_texture_cube = nullptr;
  mesh_manager m_skybox_mesh_manager;
  shader *m_skybox_shader = nullptr;
  int m_test_mode = 0;
  float m_refract_ratio = 1.0f / 1.52f;
};