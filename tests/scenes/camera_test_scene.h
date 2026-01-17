#pragma once

#include "camera_scene_base.h"
#include "tests/component/mesh_manager.h"
#include "basic/shader.h"

class camera_test_scene : public camera_scene_base {
public:
  camera_test_scene();
  virtual ~camera_test_scene() = default;

  camera_test_scene(const camera_test_scene &) = delete;
  camera_test_scene &operator=(const camera_test_scene &) = delete;
  camera_test_scene(camera_test_scene &&) = delete;
  camera_test_scene &operator=(camera_test_scene &&) = delete;

public:
  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  mesh_manager m_mesh;
  shader *m_shader = nullptr;
};