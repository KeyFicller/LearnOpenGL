#pragma once

#include "scene_base.h"
#include "shader.h"
#include "texture.h"
#include <glad/gl.h>

// Texture test scene
class texture_test_scene : public test_scene_base {
public:
  texture_test_scene();
  virtual ~texture_test_scene();

  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  GLuint m_VAO;
  GLuint m_VBO;
  GLuint m_EBO;
  shader *m_shader;
  texture_2d *m_texture1;
  texture_2d *m_texture2;
  float m_mix_ratio;
};
