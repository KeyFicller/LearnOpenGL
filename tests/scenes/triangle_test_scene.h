#pragma once

#include "scene_base.h"
#include "basic/shader.h"
#include "basic/vertex_array_object.h"
#include <glad/gl.h>

// Triangle test scene
class triangle_test_scene : public test_scene_base {
public:
  triangle_test_scene();
  virtual ~triangle_test_scene();

  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  vertex_array_object *m_VAO;
  vertex_buffer_object *m_VBO;
  shader *m_shader;
  // Triangle vertex positions (x, y for each vertex)
  float m_vertices[3][2];
  // Triangle vertex colors (r, g, b for each vertex)
  float m_colors[3][3];

  // Update VBO with current vertex data
  void update_vbo();
};
