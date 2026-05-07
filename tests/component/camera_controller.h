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
  void set_scroll_sensitivity(float _sensitivity) {
    m_scroll_sensitivity = _sensitivity;
  }
  void set_pan_sensitivity(float _s) { m_pan_sensitivity = _s; }
  void set_orbit_sensitivity(float _s) { m_orbit_sensitivity = _s; }
  void set_orbit_radius(float _r) { m_orbit_radius = _r; }

  float get_movement_speed() const { return m_movement_speed; }
  float get_scroll_sensitivity() const { return m_scroll_sensitivity; }
  float get_pan_sensitivity() const { return m_pan_sensitivity; }
  float get_orbit_sensitivity() const { return m_orbit_sensitivity; }
  float get_orbit_radius() const { return m_orbit_radius; }

  // Mouse capture
  void set_mouse_captured(bool _captured);
  bool is_mouse_captured() const { return m_mouse_captured; }

private:
  /// 0 = no mouse steering, 1 = Ctrl pan, 2 = Alt orbit
  int navigation_mod_mode() const;

  camera &m_camera;
  GLFWwindow *m_window;

  double m_last_xpos = 0.0;
  double m_last_ypos = 0.0;
  bool m_first_mouse = true;
  bool m_mouse_captured = false;
  int m_prev_nav_mode = -1;

  glm::vec3 m_orbit_pivot = glm::vec3(0.0f);
  bool m_orbit_pivot_valid = false;

  /// Viewport size in pixels (orthographic pan scales with frustum / pixels).
  int m_viewport_w = 1280;
  int m_viewport_h = 720;

  // Configurable parameters
  float m_movement_speed = 2.5f;
  float m_scroll_sensitivity = 2.0f;
  float m_pan_sensitivity = 0.006f;
  float m_orbit_sensitivity = 0.18f;
  float m_orbit_radius = 4.0f;
  const float m_min_fov = 1.0f;
  const float m_max_fov = 120.0f;
};
