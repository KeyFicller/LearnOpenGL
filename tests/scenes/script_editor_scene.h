#pragma once

#include "TextEditor.h"
#include "basic/shader.h"
#include "glm/fwd.hpp"
#include "scene_base.h"
#include "scripts/mono_invoker.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/script_editor.h"

#include <string>

// Script editor scene — Mono host feeds input; C# runs a tiny collect game.
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
  void push_input_to_script();
  void pull_game_state_from_script();
  void request_reset_game();
  void sample_keyboard_axis(float &move_x, float &move_y) const;
  void cursor_to_ndc(float &ndc_x, float &ndc_y) const;

private:
  std::unique_ptr<script_editor> m_script_editor = nullptr;
  mesh_manager m_mesh_manager;
  shader *m_shader = nullptr;
  mono_invoker::invoker m_invoker;

  glm::vec2 m_player = glm::vec2(0.0f);
  glm::vec2 m_target = glm::vec2(0.45f, 0.35f);
  int m_score = 0;
  bool m_won = false;

  // Viewport-relative cursor (pixels, y down) + button edges
  float m_cursor_x = 0.0f;
  float m_cursor_y = 0.0f;
  bool m_left_down = false;
  bool m_right_down = false;
  bool m_left_clicked = false;
  bool m_right_clicked = false;
  bool m_r_was_down = false;

  bool m_last_invoke_ok = true;
  std::string m_last_invoke_msg = "idle";
};
