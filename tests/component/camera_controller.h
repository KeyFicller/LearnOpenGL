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

private:
  camera &m_camera;
  GLFWwindow *m_window;

  double m_last_xpos = 0.0;
  double m_last_ypos = 0.0;
  bool m_first_mouse = true;
};

