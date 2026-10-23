#pragma once

#include "TextEditor.h"
#include "basic/shader.h"
#include "glm/fwd.hpp"
#include "scene_base.h"
#include "scripts/mono_invoker.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/script_editor.h"

#include <string>

// Script editor scene — teachable C++ Mono host ↔ C# frame loop.
class script_editor_scene : public test_scene_base {
public:
  script_editor_scene();
  virtual ~script_editor_scene();

private:
  void init(GLFWwindow *_window) override;
  void update(float _delta_time) override;
  void render() override;
  void render_ui() override;

  bool on_save_script();
  void sync_path_radius_to_script();
  void note_invoke(bool ok, const char *what);

private:
  std::unique_ptr<script_editor> m_script_editor = nullptr;
  mesh_manager m_mesh_manager;
  shader *m_shader = nullptr;
  mono_invoker::invoker m_invoker;
  glm::vec2 m_offset = glm::vec2(0.0f, 0.0f);
  float m_path_radius = 0.45f;
  bool m_last_invoke_ok = true;
  std::string m_last_invoke_msg = "idle";
};
