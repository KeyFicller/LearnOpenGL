#pragma once

#include "camera.h"
#include "scene_base.h"
#include "test_component/camera_controller.h"

// Base class for scenes that need camera functionality
class camera_scene_base : public test_scene_base {
public:
  camera_scene_base(const std::string &_name);
  virtual ~camera_scene_base();

  // Initialize camera and camera controller
  void init(GLFWwindow *_window) override;

  // Update camera controller
  void update(float _delta_time) override;

  // Event handlers for camera control
  bool on_mouse_moved(double _xpos, double _ypos) override;
  bool on_mouse_scroll(double _xoffset, double _yoffset) override;

  // Render camera controller UI (checkbox)
  // Subclasses should call this in their render_ui() implementation
  void render_camera_ui();

protected:
  camera m_camera;
  camera_controller *m_camera_controller = nullptr;
  bool m_camera_controller_enabled = false;
};

