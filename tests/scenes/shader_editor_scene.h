#pragma once

#include "TextEditor.h"
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

  bool compile_and_replace_shader(std::string *_error_message = nullptr);

private:
  void update_autocomplete(const std::string &prefix);
  void show_autocomplete_popup();
  void insert_completion(const std::string &completion);
  std::string get_current_word();
  bool on_key_pressed_with_popup();

  TextEditor m_editor;
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

  bool m_is_editing_fragment_shader = false;
  std::string m_edit_hint = "Ctrl+S to save";

  // Autocomplete
  bool m_show_autocomplete = false;
  std::vector<std::string> m_completions;
  int m_selected_completion = 0;
  std::string m_current_word;
};