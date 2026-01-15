#pragma once

#include "camera_scene_base.h"
#include "shader.h"
#include "vertex_array_object.h"

class camera_test_scene : public camera_scene_base {
public:
  camera_test_scene();
  virtual ~camera_test_scene();

  camera_test_scene(const camera_test_scene &) = delete;
  camera_test_scene &operator=(const camera_test_scene &) = delete;
  camera_test_scene(camera_test_scene &&) = delete;
  camera_test_scene &operator=(camera_test_scene &&) = delete;

public:
  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  vertex_array_object *m_VAO = nullptr;
  vertex_buffer_object *m_VBO = nullptr;
  index_buffer_object *m_EBO = nullptr;
  shader *m_shader = nullptr;
};