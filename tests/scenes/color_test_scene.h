#pragma once

#include "scene_base.h"
#include "shader.h"
#include "vertex_array_object.h"

// Color test scene
class color_test_scene : public test_scene_base {
public:
  color_test_scene();
  virtual ~color_test_scene();

  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  vertex_array_object *m_VAO = nullptr;
  vertex_buffer_object *m_VBO = nullptr;
  index_buffer_object *m_EBO = nullptr;
  shader *m_shader = nullptr;
  // Color control (RGB)
  float m_color[3];
  // Animation toggle
  bool m_animate;
};
