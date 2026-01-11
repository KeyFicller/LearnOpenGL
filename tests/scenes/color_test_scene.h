#pragma once

#include "scene_base.h"
#include "shader.h"
#include <glad/gl.h>

// Color test scene
class color_test_scene : public test_scene_base {
public:
  color_test_scene();
  virtual ~color_test_scene();

  void init() override;
  void render() override;
  void render_ui() override;

private:
  GLuint m_VAO;
  GLuint m_VBO;
  GLuint m_EBO;
  shader *m_shader;
  // Color control (RGB)
  float m_color[3];
  // Animation toggle
  bool m_animate;
};
