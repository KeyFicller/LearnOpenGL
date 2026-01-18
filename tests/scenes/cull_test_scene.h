#pragma once

#include "renderable_scene_base.h"

class cull_test_scene : public renderable_scene_base {
public:
  cull_test_scene();
  virtual ~cull_test_scene() = default;

  void init(GLFWwindow *window) override;
  void render() override;
  void render_ui() override;

protected:
  bool m_enable_culling = false;
  bool m_cull_back_faces = true;
};