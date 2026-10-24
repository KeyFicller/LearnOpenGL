#include "script_editor_scene.h"

#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/framework/test_suit.h"
#include <glm/glm.hpp>

#include <GLFW/glfw3.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

static const std::string gs_script_path = "scripts/csharp/ExampleScript.cs";

static std::string script_dll_path() {
  const char *candidates[] = {"runtime/Scripts.dll",
                              "scripts/csharp/bin/Scripts.dll"};
  for (const char *p : candidates) {
    std::ifstream f(p);
    if (f.good())
      return p;
  }
  return "runtime/Scripts.dll";
}

// Unit quad in local space; uScale shrinks it in the vertex shader.
static float quad_vertices[] = {-0.5f, -0.5f, 0.0f, 0.5f,  -0.5f, 0.0f,
                                0.5f,  0.5f,  0.0f, -0.5f, 0.5f,  0.0f};

static unsigned int quad_indices[] = {0, 1, 2, 2, 3, 0};

script_editor_scene::script_editor_scene() : test_scene_base("Script Editor") {
  m_script_editor = std::make_unique<script_editor>("Script Editor");
  m_script_editor->set_save_callback([this]() { return on_save_script(); });
  m_script_editor->set_help_info(
      "Game loop: host Input.SetFrame + Time.SetFrame, then "
      "ExampleScript.Update (Start on load/reset). "
      "[ScriptField] fields are discovered via ScriptFieldMirror; Inspector "
      "builds widgets from reflection. Edit .cs + Save (macOS hot-reload) "
      "keeps live inspector tweaks by name+kind; Refresh fields from script "
      "re-pulls CS defaults. WASD move, click or walk into target.");
}

script_editor_scene::~script_editor_scene() { delete m_shader; }

void script_editor_scene::note_invoke(bool ok, const char *what) {
  m_last_invoke_ok = ok;
  if (!ok) {
    m_last_invoke_msg = std::string(what) + ": failed";
  }
}

int script_editor_scene::find_field(const char *name) const {
  if (!name)
    return -1;
  for (size_t i = 0; i < m_fields.size(); ++i) {
    if (m_fields[i].name == name)
      return static_cast<int>(i);
  }
  return -1;
}

void script_editor_scene::init(GLFWwindow *_window) {
  test_scene_base::init(_window);

  mesh_data data(quad_vertices, sizeof(quad_vertices), quad_indices,
                 sizeof(quad_indices) / sizeof(unsigned int),
                 {{3, GL_FLOAT, GL_FALSE}});
  m_mesh_manager.setup_mesh(data);

  m_shader = new shader("shaders/script_editor_test/vertex.shader",
                        "shaders/script_editor_test/fragment.shader");

  std::ifstream fin(gs_script_path);
  std::stringstream ss;
  ss << fin.rdbuf();
  m_script_editor->set_text(ss.str());

  m_invoker.load(script_dll_path());
  if (m_invoker.is_ready()) {
    request_reset_game();
    refresh_script_fields();
    m_last_invoke_msg = "assembly ready";
    m_last_invoke_ok = true;
  } else {
    m_last_invoke_msg = "assembly not loaded";
    m_last_invoke_ok = false;
    m_script_editor->set_help_info("Failed to load Scripts.dll");
  }
}

void script_editor_scene::resolve_viewport_size(int &w, int &h) const {
  w = 0;
  h = 0;
  if (!m_window)
    return;
  test_suit *ts = static_cast<test_suit *>(glfwGetWindowUserPointer(m_window));
  if (!ts || !ts->m_scene_framebuffer)
    return;
  w = ts->m_scene_framebuffer->get_width();
  h = ts->m_scene_framebuffer->get_height();
}

bool script_editor_scene::viewport_has_focus() const {
  if (!m_window)
    return false;
  test_suit *ts = static_cast<test_suit *>(glfwGetWindowUserPointer(m_window));
  return ts && ts->m_viewport_focused;
}

void script_editor_scene::forward_input_to_script() {
  if (!m_invoker.is_ready())
    return;

  int vw = 0, vh = 0;
  resolve_viewport_size(vw, vh);
  m_input.sample(m_window, vw, vh, !viewport_has_focus());

  if (m_input.reset_pressed()) {
    request_reset_game();
  }

  float move_x = m_input.move_x();
  float move_y = m_input.move_y();
  float mouse_x = m_input.mouse_ndc_x();
  float mouse_y = m_input.mouse_ndc_y();
  int buttons = m_input.mouse_buttons();
  int clicked = m_input.mouse_clicked();

  // Host writes Input directly (Unity-style); ExampleScript only reads it.
  bool ok =
      m_invoker.invoke(mono_invoker::script_ncm(
                           "Scripts"_ns, "Input"_cls, "SetFrame"_md),
                       move_x, move_y, mouse_x, mouse_y, buttons, clicked);
  note_invoke(ok, "Input.SetFrame");

  m_input.clear_frame_edges();
}

void script_editor_scene::pull_render_state_from_script() {
  if (!m_invoker.is_ready())
    return;

  bool ok = true;
  ok = m_invoker.invoke_r(mono_invoker::script_ncm(
                              "Scripts"_ns, "ExampleScript"_cls, "PlayerX"_md),
                          m_player.x) &&
       ok;
  ok = m_invoker.invoke_r(mono_invoker::script_ncm(
                              "Scripts"_ns, "ExampleScript"_cls, "PlayerY"_md),
                          m_player.y) &&
       ok;
  ok = m_invoker.invoke_r(mono_invoker::script_ncm(
                              "Scripts"_ns, "ExampleScript"_cls, "TargetX"_md),
                          m_target.x) &&
       ok;
  ok = m_invoker.invoke_r(mono_invoker::script_ncm(
                              "Scripts"_ns, "ExampleScript"_cls, "TargetY"_md),
                          m_target.y) &&
       ok;
  ok = m_invoker.invoke_r(mono_invoker::script_ncm(
                              "Scripts"_ns, "ExampleScript"_cls, "Score"_md),
                          m_score) &&
       ok;
  int won = 0;
  ok = m_invoker.invoke_r(mono_invoker::script_ncm(
                              "Scripts"_ns, "ExampleScript"_cls, "IsWon"_md),
                          won) &&
       ok;
  m_won = won != 0;
  note_invoke(ok, "pull render state");
}

void script_editor_scene::refresh_script_fields() {
  if (!m_invoker.is_ready())
    return;

  // RefreshExample binds Scripts.ExampleScript (avoids MonoString arg plumbing).
  bool ok = m_invoker.invoke(mono_invoker::script_ncm(
      "Scripts"_ns, "ScriptFieldMirror"_cls, "RefreshExample"_md));
  note_invoke(ok, "ScriptFieldMirror.RefreshExample");
  if (!ok)
    return;

  int count = 0;
  ok = m_invoker.invoke_r(mono_invoker::script_ncm(
                              "Scripts"_ns, "ScriptFieldMirror"_cls, "Count"_md),
                          count) &&
       ok;
  if (!ok) {
    note_invoke(false, "ScriptFieldMirror.Count");
    return;
  }

  m_fields.clear();
  m_fields.reserve(static_cast<size_t>(count > 0 ? count : 0));

  for (int i = 0; i < count; ++i) {
    script_field_entry e;
    std::string name;
    int kind = -1;
    float min_v = 0.0f, max_v = 0.0f;
    int has_range = 0;

    bool row_ok = true;
    row_ok = m_invoker.invoke_r(
                 mono_invoker::script_ncm("Scripts"_ns, "ScriptFieldMirror"_cls,
                                          "GetName"_md),
                 name, i) &&
             row_ok;
    row_ok = m_invoker.invoke_r(
                 mono_invoker::script_ncm("Scripts"_ns, "ScriptFieldMirror"_cls,
                                          "GetKind"_md),
                 kind, i) &&
             row_ok;
    row_ok = m_invoker.invoke_r(
                 mono_invoker::script_ncm("Scripts"_ns, "ScriptFieldMirror"_cls,
                                          "GetMin"_md),
                 min_v, i) &&
             row_ok;
    row_ok = m_invoker.invoke_r(
                 mono_invoker::script_ncm("Scripts"_ns, "ScriptFieldMirror"_cls,
                                          "GetMax"_md),
                 max_v, i) &&
             row_ok;
    row_ok = m_invoker.invoke_r(
                 mono_invoker::script_ncm("Scripts"_ns, "ScriptFieldMirror"_cls,
                                          "HasRange"_md),
                 has_range, i) &&
             row_ok;

    if (!row_ok || kind < 0) {
      note_invoke(false, "ScriptFieldMirror metadata");
      continue;
    }

    e.name = name;
    e.kind = kind;
    e.min_v = min_v;
    e.max_v = max_v;
    e.has_range = has_range != 0;
    m_fields.push_back(e);
  }

  pull_scriptable_fields();
  apply_render_from_fields();
  note_invoke(true, "fields discovered");
  m_last_invoke_msg =
      "discovered " + std::to_string(m_fields.size()) + " [ScriptField]s";
}

void script_editor_scene::stash_script_fields() {
  // Snapshot live inspector values before unload/reload. Keyed later by
  // name + kind so renamed / type-changed fields are skipped cleanly.
  m_field_stash = m_fields;
}

void script_editor_scene::restore_script_fields_from_stash() {
  if (m_field_stash.empty() || m_fields.empty()) {
    m_field_stash.clear();
    return;
  }

  int restored = 0;
  for (script_field_entry &e : m_fields) {
    const script_field_entry *hit = nullptr;
    for (const script_field_entry &s : m_field_stash) {
      if (s.name == e.name && s.kind == e.kind) {
        hit = &s;
        break;
      }
    }
    if (!hit)
      continue; // missing, renamed, or type-changed — keep CS default

    if (e.kind == 0)
      e.f = hit->f;
    else if (e.kind == 1)
      e.i = hit->i;
    else if (e.kind == 2)
      e.b = hit->b;
    else if (e.kind == 3) {
      e.color[0] = hit->color[0];
      e.color[1] = hit->color[1];
      e.color[2] = hit->color[2];
    }
    ++restored;
  }

  m_field_stash.clear();
  if (restored == 0)
    return;

  push_scriptable_fields();
  // Re-pull so C# clamps (min/max) show in the panel.
  pull_scriptable_fields();
  m_last_invoke_msg = "reloaded; restored " + std::to_string(restored) +
                      " inspector override(s)";
}

void script_editor_scene::pull_scriptable_fields() {
  if (!m_invoker.is_ready())
    return;

  bool ok = true;
  for (size_t i = 0; i < m_fields.size(); ++i) {
    script_field_entry &e = m_fields[i];
    int idx = static_cast<int>(i);
    if (e.kind == 0) {
      ok = m_invoker.invoke_r(
               mono_invoker::script_ncm("Scripts"_ns, "ScriptFieldMirror"_cls,
                                        "GetFloat"_md),
               e.f, idx) &&
           ok;
    } else if (e.kind == 1) {
      ok = m_invoker.invoke_r(
               mono_invoker::script_ncm("Scripts"_ns, "ScriptFieldMirror"_cls,
                                        "GetInt"_md),
               e.i, idx) &&
           ok;
    } else if (e.kind == 2) {
      int bv = 0;
      ok = m_invoker.invoke_r(
               mono_invoker::script_ncm("Scripts"_ns, "ScriptFieldMirror"_cls,
                                        "GetBool"_md),
               bv, idx) &&
           ok;
      e.b = bv != 0;
    } else if (e.kind == 3) {
      ok = m_invoker.invoke_r(
               mono_invoker::script_ncm("Scripts"_ns, "ScriptFieldMirror"_cls,
                                        "GetColorR"_md),
               e.color[0], idx) &&
           ok;
      ok = m_invoker.invoke_r(
               mono_invoker::script_ncm("Scripts"_ns, "ScriptFieldMirror"_cls,
                                        "GetColorG"_md),
               e.color[1], idx) &&
           ok;
      ok = m_invoker.invoke_r(
               mono_invoker::script_ncm("Scripts"_ns, "ScriptFieldMirror"_cls,
                                        "GetColorB"_md),
               e.color[2], idx) &&
           ok;
    }
  }
  note_invoke(ok, "pull scriptable fields");
  apply_render_from_fields();
}

void script_editor_scene::push_scriptable_fields() {
  if (!m_invoker.is_ready())
    return;

  bool ok = true;
  for (size_t i = 0; i < m_fields.size(); ++i) {
    script_field_entry &e = m_fields[i];
    int idx = static_cast<int>(i);
    if (e.kind == 0) {
      ok = m_invoker.invoke(mono_invoker::script_ncm(
                                "Scripts"_ns, "ScriptFieldMirror"_cls,
                                "SetFloat"_md),
                            idx, e.f) &&
           ok;
    } else if (e.kind == 1) {
      ok = m_invoker.invoke(mono_invoker::script_ncm(
                                "Scripts"_ns, "ScriptFieldMirror"_cls,
                                "SetInt"_md),
                            idx, e.i) &&
           ok;
    } else if (e.kind == 2) {
      int bv = e.b ? 1 : 0;
      ok = m_invoker.invoke(mono_invoker::script_ncm(
                                "Scripts"_ns, "ScriptFieldMirror"_cls,
                                "SetBool"_md),
                            idx, bv) &&
           ok;
    } else if (e.kind == 3) {
      ok = m_invoker.invoke(mono_invoker::script_ncm(
                                "Scripts"_ns, "ScriptFieldMirror"_cls,
                                "SetColor"_md),
                            idx, e.color[0], e.color[1], e.color[2]) &&
           ok;
    }
  }
  note_invoke(ok, "push scriptable fields");
}

void script_editor_scene::apply_render_from_fields() {
  int i;
  if ((i = find_field("PlayerHalf")) >= 0 && m_fields[i].kind == 0)
    m_player_half = m_fields[i].f;
  if ((i = find_field("TargetHalf")) >= 0 && m_fields[i].kind == 0)
    m_target_half = m_fields[i].f;
  if ((i = find_field("GoalScore")) >= 0 && m_fields[i].kind == 1)
    m_goal_score = m_fields[i].i;
  if ((i = find_field("PlayerColor")) >= 0 && m_fields[i].kind == 3) {
    m_player_color[0] = m_fields[i].color[0];
    m_player_color[1] = m_fields[i].color[1];
    m_player_color[2] = m_fields[i].color[2];
  }
  if ((i = find_field("TargetColor")) >= 0 && m_fields[i].kind == 3) {
    m_target_color[0] = m_fields[i].color[0];
    m_target_color[1] = m_fields[i].color[1];
    m_target_color[2] = m_fields[i].color[2];
  }
}

void script_editor_scene::request_reset_game() {
  if (!m_invoker.is_ready())
    return;
  bool ok = m_invoker.invoke(mono_invoker::script_ncm(
      "Scripts"_ns, "ExampleScript"_cls, "Start"_md));
  note_invoke(ok, "Start");
  pull_render_state_from_script();
  if (ok)
    m_last_invoke_msg = "game reset (Start)";
}

void script_editor_scene::update(float _delta_time) {
  if (!m_invoker.is_ready())
    return;

  // 1) Input.SetFrame  2) Time.SetFrame  3) ExampleScript.Update  4) Pull
  forward_input_to_script();

  float dt = _delta_time;
  bool ok =
      m_invoker.invoke(mono_invoker::script_ncm(
                           "Scripts"_ns, "Time"_cls, "SetFrame"_md),
                       dt);
  note_invoke(ok, "Time.SetFrame");

  ok = m_invoker.invoke(mono_invoker::script_ncm(
           "Scripts"_ns, "ExampleScript"_cls, "Update"_md)) &&
       ok;
  note_invoke(ok, "Update");

  pull_render_state_from_script();
}

void script_editor_scene::render() {
  if (!m_shader)
    return;

  m_shader->use();

  // Target then player — pose + scale + color come from discovered fields.
  m_shader->set_uniform("uOffset", m_target);
  m_shader->set_uniform("uScale", m_target_half * 2.0f);
  m_shader->set_uniform(
      "uColor",
      glm::vec3(m_target_color[0], m_target_color[1], m_target_color[2]));
  m_mesh_manager.draw();

  m_shader->set_uniform("uOffset", m_player);
  m_shader->set_uniform("uScale", m_player_half * 2.0f);
  m_shader->set_uniform(
      "uColor",
      glm::vec3(m_player_color[0], m_player_color[1], m_player_color[2]));
  m_mesh_manager.draw();
}

void script_editor_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Script Game Host");
  ImGui::Spacing();
  ImGui::TextWrapped(
      "WASD / arrows move. Left-click the target (or walk into it) to score. "
      "R resets. [ScriptField] on ExampleScript is discovered by "
      "ScriptFieldMirror — Inspector widgets below are built from reflection, "
      "not hand-wired Get/Set. Save keeps live inspector tweaks; Refresh "
      "re-pulls CS defaults.");
  ImGui::Text("Assembly: %s", m_invoker.is_ready() ? "ready" : "not loaded");
  ImGui::Text("Last invoke: %s", m_last_invoke_msg.c_str());
  ImGui::Text("Score: %d / %d%s", m_score, m_goal_score,
              m_won ? "  YOU WIN" : "");
  ImGui::Text("Player: (%.2f, %.2f)  Target: (%.2f, %.2f)", m_player.x,
              m_player.y, m_target.x, m_target.y);
  if (ImGui::Button("Reset game")) {
    request_reset_game();
  }

  ImGui::Separator();
  ImGui::Text("Script Inspector (reflection)");
  ImGui::TextWrapped(
      "Widgets come from ScriptFieldMirror over [ScriptField] public static "
      "fields (float / int / bool / Color). Unsupported types are skipped. "
      "Save/hot-reload keeps live tweaks (match by name + kind). "
      "Refresh fields from script discards overrides and re-pulls CS defaults.");

  bool dirty = false;
  if (m_fields.empty()) {
    ImGui::TextDisabled("(no [ScriptField] discovered — Refresh after load)");
  }
  for (size_t i = 0; i < m_fields.size(); ++i) {
    script_field_entry &e = m_fields[i];
    ImGui::PushID(static_cast<int>(i));
    if (e.kind == 0) {
      float lo = e.has_range ? e.min_v : 0.0f;
      float hi = e.has_range ? e.max_v : 10.0f;
      if (e.has_range)
        dirty |= ImGui::SliderFloat(e.name.c_str(), &e.f, lo, hi, "%.2f");
      else
        dirty |= ImGui::DragFloat(e.name.c_str(), &e.f, 0.01f);
    } else if (e.kind == 1) {
      int lo = e.has_range ? static_cast<int>(e.min_v) : 0;
      int hi = e.has_range ? static_cast<int>(e.max_v) : 100;
      if (e.has_range)
        dirty |= ImGui::SliderInt(e.name.c_str(), &e.i, lo, hi);
      else
        dirty |= ImGui::DragInt(e.name.c_str(), &e.i, 1.0f);
    } else if (e.kind == 2) {
      dirty |= ImGui::Checkbox(e.name.c_str(), &e.b);
    } else if (e.kind == 3) {
      dirty |= ImGui::ColorEdit3(e.name.c_str(), e.color);
    } else {
      ImGui::TextDisabled("%s (unsupported kind %d)", e.name.c_str(), e.kind);
    }
    ImGui::PopID();
  }

  if (dirty) {
    push_scriptable_fields();
    // Re-pull so clamped values from C# show in the panel.
    pull_scriptable_fields();
    m_last_invoke_msg = "inspector pushed fields";
  }

  if (ImGui::Button("Refresh fields from script")) {
    // Explicit path: discard any pending stash and re-pull CS defaults.
    m_field_stash.clear();
    refresh_script_fields();
  }

  ImGui::Separator();
  m_script_editor->render();
}

bool script_editor_scene::on_mouse_moved(double _xpos, double _ypos) {
  m_input.on_mouse_moved(static_cast<float>(_xpos), static_cast<float>(_ypos));
  return false;
}

bool script_editor_scene::on_mouse_button(int _button, int _action, int _mods) {
  (void)_mods;
  m_input.on_mouse_button(_button, _action);
  return false;
}

bool script_editor_scene::on_save_script() {
  std::string script_content = m_script_editor->get_text();
  std::ofstream fout(gs_script_path);
  fout << script_content;
  fout.close();

#ifdef __APPLE__
  // Block on build_csharp.sh, then unload old assembly and load the new DLL.
  const char *cmds[] = {"bash scripts/build_csharp.sh 2>&1",
                        "bash ../scripts/build_csharp.sh 2>&1"};
  int status = -1;
  for (const char *cmd : cmds) {
    FILE *pipe = popen(cmd, "r");
    if (!pipe)
      continue;
    std::string output;
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe) != nullptr)
      output += buf;
    status = pclose(pipe);
    std::cout << "[Script Build] " << cmd << "\n" << output << std::flush;
    if (status == 0)
      break;
  }
  if (status != 0)
    std::cerr << "[Script Build] failed (exit " << status
              << "), run from project root or build/" << std::endl;
#else
  // TODO: Windows - 可用 _popen 或 CreateProcess + 重定向
  // Linux save-rebuild intentionally left Apple-only (see closed #12).
#endif

  // Stash live inspector overrides before the assembly goes away. After
  // reload we re-discover fields, then re-apply by name + kind (Unity-editor
  // lesson). "Refresh fields from script" is the discard-to-CS-defaults path.
  stash_script_fields();

  m_invoker.unload();
  m_invoker.load(script_dll_path());

  if (!m_invoker.is_ready()) {
    m_script_editor->set_help_info("Failed to load script");
    m_last_invoke_ok = false;
    m_last_invoke_msg = "reload failed";
    m_field_stash.clear();
  } else {
    request_reset_game();
    refresh_script_fields(); // rediscover + pull CS defaults first
    restore_script_fields_from_stash();
    m_last_invoke_ok = true;
    if (m_last_invoke_msg.rfind("reloaded; restored", 0) != 0)
      m_last_invoke_msg = "reloaded + re-discovered fields";
  }

  return true;
}
