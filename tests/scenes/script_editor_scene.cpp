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
      "Tweak fields in Script Inspector OR edit .cs + Save (macOS hot-reload). "
      "WASD move, click or walk into target.");
}

script_editor_scene::~script_editor_scene() { delete m_shader; }

void script_editor_scene::note_invoke(bool ok, const char *what) {
  m_last_invoke_ok = ok;
  if (!ok) {
    m_last_invoke_msg = std::string(what) + ": failed";
  }
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
    pull_scriptable_fields();
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

void script_editor_scene::pull_scriptable_fields() {
  if (!m_invoker.is_ready())
    return;

  bool ok = true;
  ok = m_invoker.invoke_r(
           mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                                    "GetMoveSpeed"_md),
           m_move_speed) &&
       ok;
  ok = m_invoker.invoke_r(
           mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                                    "GetPlayerHalf"_md),
           m_player_half) &&
       ok;
  ok = m_invoker.invoke_r(
           mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                                    "GetTargetHalf"_md),
           m_target_half) &&
       ok;
  ok = m_invoker.invoke_r(
           mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                                    "GetGoalScore"_md),
           m_goal_score) &&
       ok;
  ok = m_invoker.invoke_r(
           mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                                    "GetPlayerR"_md),
           m_player_color[0]) &&
       ok;
  ok = m_invoker.invoke_r(
           mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                                    "GetPlayerG"_md),
           m_player_color[1]) &&
       ok;
  ok = m_invoker.invoke_r(
           mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                                    "GetPlayerB"_md),
           m_player_color[2]) &&
       ok;
  ok = m_invoker.invoke_r(
           mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                                    "GetTargetR"_md),
           m_target_color[0]) &&
       ok;
  ok = m_invoker.invoke_r(
           mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                                    "GetTargetG"_md),
           m_target_color[1]) &&
       ok;
  ok = m_invoker.invoke_r(
           mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                                    "GetTargetB"_md),
           m_target_color[2]) &&
       ok;
  note_invoke(ok, "pull scriptable fields");
}

void script_editor_scene::push_scriptable_fields() {
  if (!m_invoker.is_ready())
    return;

  bool ok = true;
  ok = m_invoker.invoke(mono_invoker::script_ncm(
                            "Scripts"_ns, "ExampleScript"_cls, "SetMoveSpeed"_md),
                        m_move_speed) &&
       ok;
  ok = m_invoker.invoke(mono_invoker::script_ncm(
                            "Scripts"_ns, "ExampleScript"_cls, "SetPlayerHalf"_md),
                        m_player_half) &&
       ok;
  ok = m_invoker.invoke(mono_invoker::script_ncm(
                            "Scripts"_ns, "ExampleScript"_cls, "SetTargetHalf"_md),
                        m_target_half) &&
       ok;
  ok = m_invoker.invoke(mono_invoker::script_ncm(
                            "Scripts"_ns, "ExampleScript"_cls, "SetGoalScore"_md),
                        m_goal_score) &&
       ok;
  ok = m_invoker.invoke(
           mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                                    "SetPlayerColor"_md),
           m_player_color[0], m_player_color[1], m_player_color[2]) &&
       ok;
  ok = m_invoker.invoke(
           mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                                    "SetTargetColor"_md),
           m_target_color[0], m_target_color[1], m_target_color[2]) &&
       ok;
  note_invoke(ok, "push scriptable fields");
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

  // Target then player — pose + scale + color come from C# scriptable fields.
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
      "R resets. Edit ExampleScript.cs (Start/Update + scriptable fields) then "
      "Save to hot-reload on macOS — or tweak live in Script Inspector below. "
      "Host sets Input/Time and pulls pose/HUD/fields; game logic lives in C#.");
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
  ImGui::Text("Script Inspector");
  ImGui::TextWrapped(
      "Live mirror of ExampleScript scriptable fields. Drag here to push into "
      "the loaded assembly, or change defaults in .cs and Save (macOS) to "
      "reload — both paths are the teaching point.");

  bool dirty = false;
  dirty |= ImGui::SliderFloat("Move speed", &m_move_speed, 0.05f, 8.0f, "%.2f");
  dirty |= ImGui::SliderFloat("Player half", &m_player_half, 0.04f, 0.4f, "%.2f");
  dirty |= ImGui::SliderFloat("Target half", &m_target_half, 0.03f, 0.3f, "%.2f");
  dirty |= ImGui::SliderInt("Goal score", &m_goal_score, 1, 50);
  dirty |= ImGui::ColorEdit3("Player color", m_player_color);
  dirty |= ImGui::ColorEdit3("Target color", m_target_color);

  if (dirty) {
    push_scriptable_fields();
    // Re-pull so clamped values from C# show in the panel.
    pull_scriptable_fields();
    m_last_invoke_msg = "inspector pushed fields";
  }

  if (ImGui::Button("Refresh fields from script")) {
    pull_scriptable_fields();
    m_last_invoke_msg = "fields refreshed from C#";
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

  m_invoker.unload();
  m_invoker.load(script_dll_path());

  if (!m_invoker.is_ready()) {
    m_script_editor->set_help_info("Failed to load script");
    m_last_invoke_ok = false;
    m_last_invoke_msg = "reload failed";
  } else {
    request_reset_game();
    pull_scriptable_fields();
    m_last_invoke_ok = true;
    m_last_invoke_msg = "reloaded";
  }

  return true;
}
