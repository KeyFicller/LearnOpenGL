#pragma once

#include "TextEditor.h"
#include "basic/shader.h"
#include "basic/texture.h"
#include "scene_base.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/shader_editor.h"

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class shader_editor_chat;
class shader_editor_mcp;

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

  // The MCP server calls these. Editor and GL state are only touched on the
  // thread that renders this scene; on_render_thread posts there and waits.
  std::string active_project() const { return m_active_project; }
  const std::vector<std::string> &project_names() const { return m_project_names; }
  std::string vertex_text() const;
  std::string fragment_text() const;
  void refresh_projects();
  bool apply_shader(const std::string &vertex, const std::string &fragment,
                    std::string *error);
  bool open_project(const std::string &name, std::string *error);

  struct builtin_uniform_sample {
    float time = 0.0f;
    float random = 0.0f;
    float mouse_x = 0.0f;
    float mouse_y = 0.0f;
    float resolution_x = 0.0f;
    float resolution_y = 0.0f;
    std::string texture_label;
    int texture_w = 0;
    int texture_h = 0;
  };
  builtin_uniform_sample sample_builtin_uniforms();

  struct preview {
    bool ok = false;
    std::string error;
    int width = 0;
    int height = 0;
    std::string path;
    std::string mean_rgb;
    std::string grid;
  };
  // Blocks until this scene has drawn a frame, then reads that framebuffer.
  preview capture_preview();

  template <typename F> auto on_render_thread(F fn) -> decltype(fn()) {
    using R = decltype(fn());
    auto prom = std::make_shared<std::promise<R>>();
    {
      std::lock_guard<std::mutex> lock(m_mcp_mu);
      m_mcp_tasks.push_back([fn = std::move(fn), prom]() mutable {
        try {
          if constexpr (std::is_void_v<R>) {
            fn();
            prom->set_value();
          } else {
            prom->set_value(fn());
          }
        } catch (...) {
          prom->set_exception(std::current_exception());
        }
      });
    }
    auto fut = prom->get_future();
    if (fut.wait_for(std::chrono::seconds(3)) != std::future_status::ready) {
      throw std::runtime_error(
          "Shader Editor scene is not rendering. Select it in Scene Selection.");
    }
    return fut.get();
  }

  int mcp_port() const;

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

  void drain_mcp_tasks();
  void finish_capture();
  preview read_framebuffer();

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

  std::unique_ptr<shader_editor_chat> m_chat;

  std::unique_ptr<shader_editor_mcp> m_mcp;
  std::mutex m_mcp_mu;
  std::vector<std::function<void()>> m_mcp_tasks;
  std::shared_ptr<std::promise<preview>> m_capture;
};
