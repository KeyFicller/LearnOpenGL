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
#include <vector>

// Host-side mirror of one [ScriptField] discovered via ScriptFieldMirror.
struct script_field_entry {
  std::string name;
  int kind = 0; // 0 float, 1 int, 2 bool, 3 color
  float min_v = 0.0f;
  float max_v = 0.0f;
  bool has_range = false;
  float f = 0.0f;
  int i = 0;
  bool b = false;
  float color[3] = {1.0f, 1.0f, 1.0f};
};

// Script editor scene — host sets Input/Time each frame, then calls C# Start/Update;
// C# owns game state + [ScriptField] surface; host discovers fields via reflection
// and builds the ImGui Script Inspector dynamically (no per-field C++ glue).
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
  void refresh_script_fields();
  void stash_script_fields();
  void restore_script_fields_from_stash();
  void pull_scriptable_fields();
  void push_scriptable_fields();
  void apply_render_from_fields();
  void request_reset_game();
  void resolve_viewport_size(int &w, int &h) const;
  bool viewport_has_focus() const;
  int find_field(const char *name) const;

private:
  std::unique_ptr<script_editor> m_script_editor = nullptr;
  mesh_manager m_mesh_manager;
  shader *m_shader = nullptr;
  mono_invoker::invoker m_invoker;

  // Host-side input recorder (not game state). Written via Input.SetFrame.
  script_input_content m_input;

  // Render snapshot pulled from C# after Update (source of truth is .cs).
  glm::vec2 m_player = glm::vec2(0.0f);
  glm::vec2 m_target = glm::vec2(0.45f, 0.35f);
  int m_score = 0;
  bool m_won = false;

  // Dynamic mirror of [ScriptField] entries (built by ScriptFieldMirror).
  std::vector<script_field_entry> m_fields;
  // Live inspector values stashed across Save/hot-reload (keyed by name+kind).
  // Explicit "Refresh fields from script" discards this and re-pulls CS defaults.
  std::vector<script_field_entry> m_field_stash;

  // Render caches filled from discovered fields by name.
  float m_player_half = 0.12f;
  float m_target_half = 0.08f;
  int m_goal_score = 5;
  float m_player_color[3] = {0.95f, 0.35f, 0.3f};
  float m_target_color[3] = {0.2f, 0.85f, 0.9f};

  bool m_last_invoke_ok = true;
  std::string m_last_invoke_msg = "idle";
};
