#include "script_editor_scene.h"

#include "imgui.h"
#include <glm/glm.hpp>
#include "tests/component/mesh_manager.h"
#include "tests/framework/test_suit.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
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
      "Game loop: host SetInput(axis, mouse NDC, buttons) then "
      "OnUpdate(dt); script writes Player/Target/Score. "
      "WASD move, click target to collect. Save rebuilds on macOS only.");
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

void script_editor_scene::sample_keyboard_axis(float &move_x,
                                               float &move_y) const {
  move_x = 0.0f;
  move_y = 0.0f;
  if (!m_window)
    return;
  // Don't steal keys while the C# editor (or other ImGui) wants them.
  if (ImGui::GetIO().WantCaptureKeyboard)
    return;

  const bool left = glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS ||
                    glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS;
  const bool right = glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS ||
                     glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS;
  const bool down = glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS ||
                    glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS;
  const bool up = glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS ||
                  glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS;

  if (left)
    move_x -= 1.0f;
  if (right)
    move_x += 1.0f;
  if (down)
    move_y -= 1.0f;
  if (up)
    move_y += 1.0f;

  // Normalize diagonal
  const float len2 = move_x * move_x + move_y * move_y;
  if (len2 > 1.0f) {
    const float inv = 1.0f / std::sqrt(len2);
    move_x *= inv;
    move_y *= inv;
  }
}

void script_editor_scene::cursor_to_ndc(float &ndc_x, float &ndc_y) const {
  ndc_x = 0.0f;
  ndc_y = 0.0f;
  if (!m_window)
    return;
  test_suit *ts =
      static_cast<test_suit *>(glfwGetWindowUserPointer(m_window));
  if (!ts || !ts->m_scene_framebuffer)
    return;
  const int w = ts->m_scene_framebuffer->get_width();
  const int h = ts->m_scene_framebuffer->get_height();
  if (w <= 0 || h <= 0)
    return;
  // Viewport mouse is top-left origin; clip NDC is y-up.
  ndc_x = (m_cursor_x / static_cast<float>(w)) * 2.0f - 1.0f;
  ndc_y = 1.0f - (m_cursor_y / static_cast<float>(h)) * 2.0f;
  ndc_x = std::clamp(ndc_x, -1.0f, 1.0f);
  ndc_y = std::clamp(ndc_y, -1.0f, 1.0f);
}

void script_editor_scene::push_input_to_script() {
  if (!m_invoker.is_ready())
    return;

  float move_x = 0.0f;
  float move_y = 0.0f;
  sample_keyboard_axis(move_x, move_y);

  // R = reset (edge; ignored while ImGui wants keyboard)
  const bool r_down =
      m_window && !ImGui::GetIO().WantCaptureKeyboard &&
      glfwGetKey(m_window, GLFW_KEY_R) == GLFW_PRESS;
  if (r_down && !m_r_was_down) {
    request_reset_game();
  }
  m_r_was_down = r_down;

  float mouse_x = 0.0f;
  float mouse_y = 0.0f;
  cursor_to_ndc(mouse_x, mouse_y);

  int buttons = 0;
  if (m_left_down)
    buttons |= 1;
  if (m_right_down)
    buttons |= 2;
  int clicked = 0;
  if (m_left_clicked)
    clicked |= 1;
  if (m_right_clicked)
    clicked |= 2;

  bool ok = m_invoker.invoke(
      mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                               "SetInput"_md),
      move_x, move_y, mouse_x, mouse_y, buttons, clicked);
  note_invoke(ok, "SetInput");

  m_left_clicked = false;
  m_right_clicked = false;
}

void script_editor_scene::pull_game_state_from_script() {
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
  note_invoke(ok, "pull state");
}

void script_editor_scene::request_reset_game() {
  if (!m_invoker.is_ready())
    return;
  bool ok = m_invoker.invoke(mono_invoker::script_ncm(
      "Scripts"_ns, "ExampleScript"_cls, "ResetGame"_md));
  note_invoke(ok, "ResetGame");
  pull_game_state_from_script();
  if (ok)
    m_last_invoke_msg = "game reset";
}

void script_editor_scene::update(float _delta_time) {
  if (!m_invoker.is_ready())
    return;

  push_input_to_script();

  float dt = _delta_time;
  bool ok = m_invoker.invoke(
      mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                               "OnUpdate"_md),
      dt);
  note_invoke(ok, "OnUpdate");

  pull_game_state_from_script();
}

void script_editor_scene::render() {
  if (!m_shader)
    return;

  m_shader->use();

  // Target (cyan), then player (coral) on top
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
      "WASD / arrows move the red player. Left-click the cyan target "
      "(or walk into it) to score. R resets. Reach 5 to win.");
  ImGui::Text("Assembly: %s",
              m_invoker.is_ready() ? "ready" : "not loaded");
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
  m_cursor_x = static_cast<float>(_xpos);
  m_cursor_y = static_cast<float>(_ypos);
  return false;
}

bool script_editor_scene::on_mouse_button(int _button, int _action,
                                          int _mods) {
  (void)_mods;
  if (_button == GLFW_MOUSE_BUTTON_LEFT) {
    if (_action == GLFW_PRESS) {
      m_left_down = true;
      m_left_clicked = true;
    } else if (_action == GLFW_RELEASE) {
      m_left_down = false;
    }
  } else if (_button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (_action == GLFW_PRESS) {
      m_right_down = true;
      m_right_clicked = true;
    } else if (_action == GLFW_RELEASE) {
      m_right_down = false;
    }
  }
  return false;
}

bool script_editor_scene::on_save_script() {
  std::string script_content = m_script_editor->get_text();
  std::ofstream fout(gs_script_path);
  fout << script_content;
  fout.close();

#ifdef __APPLE__
  // 先阻塞执行 build_csharp.sh 生成新 DLL，再卸载旧程序集并加载新 DLL
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
