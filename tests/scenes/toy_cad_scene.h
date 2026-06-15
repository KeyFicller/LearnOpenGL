#pragma once

#include "renderable_scene_base.h"
#include <GLFW/glfw3.h>

class toy_cad_scene : public renderable_scene_base {
public:
  toy_cad_scene();
  ~toy_cad_scene() override = default;

  void init(GLFWwindow *window) override;
  void update(float delta_time) override;
  void render() override;
  void render_ui() override;
  void render_viewport_overlay() override;

  bool on_mouse_moved(double xpos, double ypos) override;
  bool on_mouse_scroll(double xoffset, double yoffset) override;
  bool on_mouse_button(int button, int action, int mods) override;
  void on_framebuffer_resized(int width, int height) override;
  void on_object_hovered(int object_id) override;
};
