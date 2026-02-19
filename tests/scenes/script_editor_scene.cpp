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
static const std::string gs_dll_path = "runtime/Scripts.dll";

static float quad_vertices[] = {-0.5f, -0.5f, 0.0f, 0.5f,  -0.5f, 0.0f,
                                0.5f,  0.5f,  0.0f, -0.5f, 0.5f,  0.0f};

static unsigned int quad_indices[] = {0, 1, 2, 2, 3, 0};

script_editor_scene::script_editor_scene() : test_scene_base("Script Editor") {
  m_script_editor = std::make_unique<script_editor>("Script Editor");
  m_script_editor->set_save_callback([this]() { return on_save_script(); });
}

script_editor_scene::~script_editor_scene() { delete m_shader; }

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

  m_invoker.load(gs_dll_path);
}

void script_editor_scene::render() {
  if (!m_shader) {
    return;
  }

  // Update offset
  if (m_invoker.is_ready()) {
    m_invoker.invoke_r(mono_invoker::script_ncm(
                           "Scripts"_ns, "ExampleScript"_cls, "OffsetX"_md),
                       m_offset.x);
    m_invoker.invoke_r(mono_invoker::script_ncm(
                           "Scripts"_ns, "ExampleScript"_cls, "OffsetY"_md),
                       m_offset.y);
  }

  m_shader->use();
  m_shader->set_uniform("uOffset", m_offset);
  m_mesh_manager.draw();
}

void script_editor_scene::render_ui() { m_script_editor->render(); }

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
    std::cerr << "[Script Build] failed (exit " << status << "), run from project root or build/" << std::endl;
#else
  // TODO: Windows - 可用 _popen 或 CreateProcess + 重定向
#endif

  m_invoker.unload();
  m_invoker.load(gs_dll_path);

  if (!m_invoker.is_ready()) {
    m_script_editor->set_help_info("Failed to load script");
  }

  return true;
}