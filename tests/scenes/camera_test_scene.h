#pragma once

#include "camera.h"
#include "scene_base.h"
#include "shader.h"
#include "test_component/camera_controller.h"
#include "vertex_array_object.h"

class camera_test_scene : public test_scene_base {
public:
  camera_test_scene();
  virtual ~camera_test_scene();

  camera_test_scene(const camera_test_scene &) = delete;
  camera_test_scene &operator=(const camera_test_scene &) = delete;
  camera_test_scene(camera_test_scene &&) = delete;
  camera_test_scene &operator=(camera_test_scene &&) = delete;

public:
  void init(GLFWwindow *_window) override;
  void update(float _delta_time) override;
  void render() override;
  void render_ui() override;
  bool on_mouse_moved(double _xpos, double _ypos) override;
  bool on_mouse_scroll(double _xoffset, double _yoffset) override;

private:
  camera m_camera;
  camera_controller *m_camera_controller = nullptr;
  vertex_array_object *m_VAO = nullptr;
  vertex_buffer_object *m_VBO = nullptr;
  index_buffer_object *m_EBO = nullptr;
  shader *m_shader = nullptr;
  bool m_enable_camera_controller = false;
};