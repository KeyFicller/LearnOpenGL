#pragma once

#include "imgui_impl_glfw.h"
#include <string>

struct GLFWwindow;

// Base class for all test scenes
class test_scene_base {
public:
  test_scene_base(const std::string &_name);
  virtual ~test_scene_base();

  // Initialize scene resources
  virtual void init(GLFWwindow *_window);

  // Update the scene
  virtual void update(float _delta_time) {}

  // Render the scene
  virtual void render() = 0;

  // Render scene-specific UI controls
  virtual void render_ui() {}

  // Get scene name
  const char *get_name() const { return m_name.c_str(); }

  // Event handlers
  virtual bool on_mouse_moved(double _xpos, double _ypos) { return false; }
  virtual bool on_mouse_scroll(double _xoffset, double _yoffset) {
    return false;
  }
  virtual void on_framebuffer_resized(int _width, int _height) {}
  virtual void on_object_hovered(int _object_id) {}
  virtual bool on_mouse_button(int _button, int _action, int _mods) {
    return false;
  }

protected:
  std::string m_name;
  GLFWwindow *m_window;
};
