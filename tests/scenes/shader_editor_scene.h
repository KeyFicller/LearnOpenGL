#pragma once

#include "TextEditor.h"
#include "basic/shader.h"
#include "basic/texture.h"
#include "scene_base.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/shader_editor.h"

#include <memory>
#include <string>
#include <vector>

// Shader editor scene: each project = subdirectory under
// shaders/shader_editor_test/
class shader_editor_scene : public test_scene_base {
public:
  shader_editor_scene();
  virtual ~shader_editor_scene();

  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

  bool compile_and_replace_shader(std::string *_error_message = nullptr);
  bool on_save_shader();

private:
  static constexpr const char *k_project_root = "shaders/shader_editor_test";
  static constexpr const char *k_default_project = "default";
  static constexpr const char *k_user_png_subdir = "textures";

  std::string vertex_path() const;
  std::string fragment_path() const;
  void refresh_project_list();
  void ensure_at_least_one_project();
  void switch_to_project(const std::string &name);
  bool try_create_project(const std::string &name, std::string *error_message);

  void reload_sources_from_disk();
  void persist_sources_to_disk() const;

  void draw_builtin_uniforms_panel();
  void open_user_texture_file_dialog();
  bool try_reload_builtin_user_png(const std::string &path_utf8);

  std::unique_ptr<shader_editor> m_vertex_shader_editor = nullptr;
  std::unique_ptr<shader_editor> m_fragment_shader_editor = nullptr;
  mesh_manager m_mesh_manager;
  shader *m_shader = nullptr;

  std::string m_vertex_shader_source;
  std::string m_fragment_shader_source;

  std::vector<std::string> m_project_names;
  std::string m_active_project;
  char m_new_project_name[128]{};
  std::string m_create_project_error;

  bool m_show_vertex_shader_editor = false;
  bool m_show_fragment_shader_editor = false;

  std::unique_ptr<texture_2d> m_builtin_user_png;
  std::string m_builtin_png_path_label;
  std::string m_builtin_png_load_error;
};
