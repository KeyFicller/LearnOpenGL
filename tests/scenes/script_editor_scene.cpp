#include "script_editor_scene.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include <algorithm>
#include <cctype>
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

static float quad_vertices[] = {-0.5f, -0.5f, 0.0f, 0.5f,  -0.5f, 0.0f,
                                0.5f,  0.5f,  0.0f, -0.5f, 0.5f,  0.0f};

static unsigned int quad_indices[] = {0, 1, 2, 2, 3, 0};

script_editor_scene::script_editor_scene() : test_scene_base("Script Editor") {
  m_script_editor = std::make_unique<script_editor>("Script Editor");
  m_script_editor->set_save_callback([this]() { return on_save_script(); });
  m_script_editor->set_help_info(
      "Host loop: update(dt) -> ExampleScript.OnUpdate -> "
      "MoveController; render reads OffsetX/Y -> uOffset. "
      "Save rebuilds Scripts.dll on macOS only.");
}

script_editor_scene::~script_editor_scene() { delete m_shader; }

void script_editor_scene::note_invoke(bool ok, const char *what) {
  m_last_invoke_ok = ok;
  if (!ok) {
    m_last_invoke_msg = std::string(what) + ": failed";
  }
}

void script_editor_scene::sync_path_radius_to_script() {
  if (!m_invoker.is_ready())
    return;
  float radius = m_path_radius;
  bool ok = m_invoker.invoke(
      mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                               "SetPathRadius"_md),
      radius);
  note_invoke(ok, "SetPathRadius");
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
  std::string script_content = ss.str();
  m_script_editor->set_text(script_content);

  m_invoker.load(script_dll_path());
  if (m_invoker.is_ready()) {
    sync_path_radius_to_script();
    m_last_invoke_msg = "assembly ready";
    m_last_invoke_ok = true;
  } else {
    m_last_invoke_msg = "assembly not loaded";
    m_last_invoke_ok = false;
    m_script_editor->set_help_info("Failed to load Scripts.dll");
  }
}

void script_editor_scene::update(float _delta_time) {
  if (!m_invoker.is_ready())
    return;

  float dt = _delta_time;
  bool ok = m_invoker.invoke(
      mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                               "OnUpdate"_md),
      dt);
  note_invoke(ok, "OnUpdate");
}

void script_editor_scene::render() {
  if (!m_shader) {
    return;
  }

  // Script-controlled offset: host already ticked OnUpdate in update().
  if (m_invoker.is_ready()) {
    bool ok_x = m_invoker.invoke_r(
        mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                                 "OffsetX"_md),
        m_offset.x);
    bool ok_y = m_invoker.invoke_r(
        mono_invoker::script_ncm("Scripts"_ns, "ExampleScript"_cls,
                                 "OffsetY"_md),
        m_offset.y);
    note_invoke(ok_x && ok_y, "OffsetX/Y");
  }

  m_shader->use();
  m_shader->set_uniform("uOffset", m_offset);
  m_mesh_manager.draw();
}

void script_editor_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Script Host");
  ImGui::Spacing();
  ImGui::Text("Assembly: %s",
              m_invoker.is_ready() ? "ready" : "not loaded");
  ImGui::Text("Active: ExampleScript -> MoveController");
  ImGui::Text("Last invoke: %s", m_last_invoke_msg.c_str());
  ImGui::Text("uOffset: (%.3f, %.3f)", m_offset.x, m_offset.y);

  if (ImGui::SliderFloat("Path radius", &m_path_radius, 0.05f, 1.0f, "%.2f")) {
    sync_path_radius_to_script();
  }

  ImGui::Separator();
  m_script_editor->render();
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
    if (!pipe) continue;
    std::string output;
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe) != nullptr)
      output += buf;
    status = pclose(pipe);
    std::cout << "[Script Build] " << cmd << "\n" << output << std::flush;
    if (status == 0) break;
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
    sync_path_radius_to_script();
    m_last_invoke_ok = true;
    m_last_invoke_msg = "reloaded";
  }

  return true;
}
