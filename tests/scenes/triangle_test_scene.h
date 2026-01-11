#pragma once

#include "scene_base.h"
#include "shader.h"
#include <glad/gl.h>

// Triangle test scene
class triangle_test_scene : public test_scene_base {
public:
  triangle_test_scene();
  virtual ~triangle_test_scene();

  void init() override;
  void render() override;
  void render_ui() override;

private:
  GLuint m_VAO;
  GLuint m_VBO;
  shader *m_shader;
  // Triangle vertex positions (x, y for each vertex)
  float m_vertices[3][2];
  // Triangle vertex colors (r, g, b for each vertex)
  float m_colors[3][3];

  // Update VBO with current vertex data
  void update_vbo();
};
