#pragma once

#include "renderable_scene_base.h"
#include "tests/component/sub_scene.h"

class spline_movement_snake_sub_scene;

class spline_movement_scene : public renderable_scene_base {
  friend class spline_movement_snake_sub_scene;

public:
  spline_movement_scene();
  virtual ~spline_movement_scene() = default;

public:
  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;
  void update(float _delta_time) override;
  bool on_mouse_moved(double _xpos, double _ypos) override;

private:
  // Sub-scene manager
  sub_scene_manager<spline_movement_scene> m_sub_scene_manager;
};