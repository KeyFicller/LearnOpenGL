#pragma once

#include "TextEditor.h"
#include "basic/shader.h"
#include "glm/fwd.hpp"
#include "scene_base.h"
#include "scripts/mono_invoker.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/script_editor.h"
#include "tests/component/script_input_content.h"

#include <string>

// Script editor scene — host records input, forwards to C#; C# owns game state;
// host pulls pose for rendering only.
class script_editor_scene : public test_scene_base {
public:
  script_editor_scene();
  virtual ~script_editor_scene();

private:
  void init(GLFWwindow *_window) override;
  void update(float _delta_time) override;
  void render() override;
  void render_ui() override;

  bool on_mouse_moved(double _xpos, double _ypos) override;
  bool on_mouse_button(int _button, int _action, int _mods) override;

  bool on_save_script();
  void note_invoke(bool ok, const char *what);
  void forward_input_to_script();
  void pull_render_state_from_script();
  void request_reset_game();
  void resolve_viewport_size(int &w, int &h) const;

private:
  std::unique_ptr<script_editor> m_script_editor = nullptr;
  mesh_manager m_mesh_manager;
  shader *m_shader = nullptr;
  mono_invoker::invoker m_invoker;

  // Host-side input recorder (not game state). Forwarded to C# HandleInput.
  script_input_content m_input;

  // Render snapshot pulled from C# after OnUpdate (source of truth is .cs).
  glm::vec2 m_player = glm::vec2(0.0f);
  glm::vec2 m_target = glm::vec2(0.45f, 0.35f);
  int m_score = 0;
  bool m_won = false;

  bool m_last_invoke_ok = true;
  std::string m_last_invoke_msg = "idle";
};
