#pragma once

#include "renderable_scene_base.h"
#include "tests/component/sub_scene.h"
#include "tests/scenes/instance_sub_scenes.h"

class instance_uniform_pass_value_scene;
class instance_attrib_pass_value_scene;

class instance_scene : public renderable_scene_base {
  friend class instance_uniform_pass_value_scene;
  friend class instance_attrib_pass_value_scene;

public:
  instance_scene();
  virtual ~instance_scene() = default;

  void init(GLFWwindow *window) override;
  void render() override;
  void render_ui() override;

protected:
  int m_row = 5;
  int m_col = 5;
  sub_scene_manager<instance_scene> m_sub_scenes;
};