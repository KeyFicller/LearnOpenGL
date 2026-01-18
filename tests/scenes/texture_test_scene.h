#pragma once

#include "scene_base.h"
#include "basic/shader.h"
#include "basic/texture.h"
#include "basic/vertex_array.h"

// Texture test scene
class texture_test_scene : public test_scene_base {
public:
  texture_test_scene();
  virtual ~texture_test_scene();

  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  vertex_array *m_VAO = nullptr;
  vertex_buffer *m_VBO = nullptr;
  index_buffer *m_EBO = nullptr;
  shader *m_shader = nullptr;
  texture_2d *m_texture1 = nullptr;
  texture_2d *m_texture2 = nullptr;
  float m_mix_ratio;
};
