#pragma once

#include "basic/shader.h"
#include "scene_base.h"
#include "tests/component/mesh_manager.h"

// Shader editor scene
class shader_editor_scene : public test_scene_base {
public:
  shader_editor_scene();
  virtual ~shader_editor_scene();

  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

  bool compile_and_replace_shader();

private:
  mesh_manager m_mesh_manager;
  shader *m_shader = nullptr;

  std::string m_vertex_shader_source = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    void main()
    {
      gl_Position = vec4(aPos, 1.0);
    }
  )";

  std::string m_fragment_shader_source = R"(
    #version 330 core
    out vec4 FragColor;
    void main()
    {
      FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
  )";
};