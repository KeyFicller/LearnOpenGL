#pragma once

#include "renderable_scene_base.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/sub_scene.h"
#include "tests/scenes/advanced_glsl_sub_scenes.h"
#include "tests/scenes/geometry_shader_sub_scenes.h"
#include <glm/glm.hpp>

class geometry_shader_sub_scene;

class geometry_shader_scene : public renderable_scene_base {
  friend class geometry_shader_sub_scene;

public:
  geometry_shader_scene();
  virtual ~geometry_shader_scene() = default;

public:
  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  // Sub-scene manager
  sub_scene_manager<geometry_shader_scene> m_sub_scene_manager;

  mesh_manager m_points_mesh_manager;
};