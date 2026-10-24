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
      "Game loop: host script_input_content -> HandleInput, then "
      "OnUpdate(dt); C# owns pose/score via Input+Time. "
      "WASD move, click or walk into target. "
      "Save rebuilds on macOS only.");
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

  bool ok =
      m_invoker.invoke(mono_invoker::script_ncm(
                           "Scripts"_ns, "ExampleScript"_cls, "HandleInput"_md),
                       move_x, move_y, mouse_x, mouse_y, buttons, clicked);
  note_invoke(ok, "HandleInput");

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

void script_editor_scene::request_reset_game() {
  if (!m_invoker.is_ready())
    return;
  bool ok = m_invoker.invoke(mono_invoker::script_ncm(
      "Scripts"_ns, "ExampleScript"_cls, "ResetGame"_md));
  note_invoke(ok, "ResetGame");
  pull_render_state_from_script();
  if (ok)
    m_last_invoke_msg = "game reset";
}

void script_editor_scene::update(float _delta_time) {
  if (!m_invoker.is_ready())
    return;

  // 1) Record + forward input content  2) C# OnUpdate  3) Pull for render
  forward_input_to_script();

  float dt = _delta_time;
  bool ok =
      m_invoker.invoke(mono_invoker::script_ncm(
                           "Scripts"_ns, "ExampleScript"_cls, "OnUpdate"_md),
                       dt);
  note_invoke(ok, "OnUpdate");

  pull_render_state_from_script();
}

void script_editor_scene::render() {
  if (!m_shader)
    return;

  m_shader->use();

  // Target (cyan), then player (coral) — poses come from C#.
  m_shader->set_uniform("uOffset", m_target);
  m_shader->set_uniform("uScale", 0.16f);
  m_shader->set_uniform("uColor", glm::vec3(0.2f, 0.85f, 0.9f));
  m_mesh_manager.draw();

  m_shader->set_uniform("uOffset", m_player);
  m_shader->set_uniform("uScale", 0.24f);
  m_shader->set_uniform("uColor", glm::vec3(0.95f, 0.35f, 0.3f));
  m_mesh_manager.draw();
}

void script_editor_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Script Game Host");
  ImGui::Spacing();
  ImGui::TextWrapped(
      "WASD / arrows move. Left-click the cyan target (or walk into it) to "
      "score. R resets. Reach 5 to win. Edit ExampleScript.cs (and Input/Time) "
      "then Save to hot-reload on macOS. Host only records input and pulls "
      "pose/HUD; game logic lives in C#.");
  ImGui::Text("Assembly: %s", m_invoker.is_ready() ? "ready" : "not loaded");
  ImGui::Text("Last invoke: %s", m_last_invoke_msg.c_str());
  ImGui::Text("Score: %d / 5%s", m_score, m_won ? "  YOU WIN" : "");
  ImGui::Text("Player: (%.2f, %.2f)  Target: (%.2f, %.2f)", m_player.x,
              m_player.y, m_target.x, m_target.y);
  if (ImGui::Button("Reset game")) {
    request_reset_game();
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
    m_last_invoke_ok = true;
    m_last_invoke_msg = "reloaded";
  }

  return true;
}
