#pragma once

#include "basic/camera.h"

struct GLFWwindow;

class camera_controller {
public:
  camera_controller(camera &_camera, GLFWwindow *window);
  virtual ~camera_controller() = default;

public:
  void update(float _delta_time);

  void on_mouse_moved(double _xpos, double _ypos);
  void on_mouse_scroll(double _xoffset, double _yoffset);
  void on_framebuffer_resized(int _width, int _height);

  // Configuration
  void set_movement_speed(float _speed) { m_movement_speed = _speed; }
  void set_mouse_sensitivity(float _sensitivity) {
    m_mouse_sensitivity = _sensitivity;
  }
  void set_scroll_sensitivity(float _sensitivity) {
    m_scroll_sensitivity = _sensitivity;
  }

  float get_movement_speed() const { return m_movement_speed; }
  float get_mouse_sensitivity() const { return m_mouse_sensitivity; }
  float get_scroll_sensitivity() const { return m_scroll_sensitivity; }

  // Mouse capture
  void set_mouse_captured(bool _captured);
  bool is_mouse_captured() const { return m_mouse_captured; }

private:
  camera &m_camera;
  GLFWwindow *m_window;

  double m_last_xpos = 0.0;
  double m_last_ypos = 0.0;
  bool m_first_mouse = true;
  bool m_mouse_captured = false;

  // Configurable parameters
  float m_movement_speed = 2.5f;
  float m_mouse_sensitivity = 0.1f;
  float m_scroll_sensitivity = 2.0f;
  const float m_max_pitch = 89.0f;
  const float m_min_fov = 1.0f;
  const float m_max_fov = 120.0f;
};

