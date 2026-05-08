#include "shader_editor_scene.h"

#include "imgui.h"

#include "tests/component/mesh_manager.h"
#include "tests/framework/test_suit.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <iterator>
#include <string>

namespace fs = std::filesystem;

namespace {

static float quad_vertices[] = {-1.0f, -1.0f, 0.0f, 1.0f,  -1.0f, 0.0f,
                                1.0f,  1.0f,  0.0f, -1.0f, 1.0f,  0.0f};

static unsigned int quad_indices[] = {0, 1, 2, 2, 3, 0};

const char k_default_vertex_shader[] = "#version 330 core\n\n"
                                       "layout(location = 0) in vec3 aPos;\n\n"
                                       "void main()\n{\n"
                                       "  gl_Position = vec4(aPos, 1.0);\n}\n";

const char k_default_fragment_shader[] =
    "#version 330 core\n\n"
    "out vec4 FragColor;\n\n"
    "void main()\n{\n"
    "  FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n}\n";

bool load_text_file(const char *path, std::string &out) {
  std::ifstream f(path);
  if (!f) {
    return false;
  }
  out.assign(std::istreambuf_iterator<char>(f),
             std::istreambuf_iterator<char>());
  return true;
}

bool save_text_file(const char *path, const std::string &contents) {
  std::ofstream f(path, std::ios::out | std::ios::trunc);
  if (!f) {
    return false;
  }
  f.write(contents.data(), static_cast<std::streamsize>(contents.size()));
  return static_cast<bool>(f);
}

bool is_valid_project_name(const std::string &s) {
  if (s.empty() || s.size() > 64U) {
    return false;
  }
  for (unsigned char uc : s) {
    if (!std::isalnum(uc) && uc != '_' && uc != '-') {
      return false;
    }
  }
  return true;
}

bool write_shaders_to_dir(const fs::path &dir, const std::string &vertex_glsl,
                          const std::string &fragment_glsl, std::string *err) {
  std::error_code ec;
  fs::create_directories(dir, ec);
  if (ec) {
    if (err) {
      *err = ec.message();
    }
    return false;
  }
  const std::string v_path = (dir / "vertex.shader").string();
  const std::string f_path = (dir / "fragment.shader").string();
  if (!save_text_file(v_path.c_str(), vertex_glsl)) {
    if (err) {
      *err = "failed to write vertex.shader";
    }
    return false;
  }
  if (!save_text_file(f_path.c_str(), fragment_glsl)) {
    if (err) {
      *err = "failed to write fragment.shader";
    }
    return false;
  }
  return true;
}

bool write_default_shaders_to_dir(const fs::path &dir, std::string *err) {
  return write_shaders_to_dir(dir, k_default_vertex_shader,
                              k_default_fragment_shader, err);
}

struct builtin_uniforms {
  float u_time = 0.0f;
  glm::vec2 u_mouse = glm::vec2(0.0f, 0.0f);
  glm::vec2 u_resolution = glm::vec2(0.0f, 0.0f);
};

static builtin_uniforms _builtin_uniforms;

void update_builtin_uniforms(GLFWwindow *window) {
  _builtin_uniforms.u_time = static_cast<float>(glfwGetTime());
  if (!window) {
    return;
  }
  test_suit *ts = static_cast<test_suit *>(glfwGetWindowUserPointer(window));
  if (!ts || !ts->m_scene_framebuffer) {
    return;
  }
  const int w = ts->m_scene_framebuffer->get_width();
  const int h = ts->m_scene_framebuffer->get_height();
  if (w <= 0 || h <= 0) {
    return;
  }
  _builtin_uniforms.u_resolution =
      glm::vec2(static_cast<float>(w), static_cast<float>(h));

  double mx = 0.0;
  double my = 0.0;
  glfwGetCursorPos(window, &mx, &my);
  const float vx = static_cast<float>(mx - ts->m_viewport_x);
  const float vy = static_cast<float>(my - ts->m_viewport_y);
  const float y_fb = static_cast<float>(h - 1) - vy;
  float nx = (vx + 0.5f) / static_cast<float>(w);
  float ny = (y_fb + 0.5f) / static_cast<float>(h);
  nx = std::clamp(nx, 0.0f, 1.0f);
  ny = std::clamp(ny, 0.0f, 1.0f);
  _builtin_uniforms.u_mouse = glm::vec2(nx, ny);
}

void apply_builtin_uniforms(shader *_shader) {
  _shader->set_uniform<float>("u_time", _builtin_uniforms.u_time);
  _shader->set_uniform<glm::vec2>("u_mouse", _builtin_uniforms.u_mouse);
  _shader->set_uniform<glm::vec2>("u_resolution",
                                  _builtin_uniforms.u_resolution);
}

void ui_builtin_uniforms() {
  ImGui::Text("u_time: %.2f", _builtin_uniforms.u_time);
  ImGui::Text("u_resolution: %.0f x %.0f", _builtin_uniforms.u_resolution.x,
              _builtin_uniforms.u_resolution.y);
  ImGui::Text("u_mouse (norm, GL Y-up): (%.3f, %.3f)",
              _builtin_uniforms.u_mouse.x, _builtin_uniforms.u_mouse.y);
}

} // namespace

shader_editor_scene::shader_editor_scene() : test_scene_base("Shader Editor") {
  m_active_project = k_default_project;
  m_vertex_shader_editor = std::make_unique<shader_editor>("Vertex Shader");
  m_fragment_shader_editor = std::make_unique<shader_editor>("Fragment Shader");
  m_vertex_shader_editor->set_save_callback(
      [this]() { return on_save_shader(); });
  m_fragment_shader_editor->set_save_callback(
      [this]() { return on_save_shader(); });

  m_vertex_shader_editor->restore_default_help_info();
  m_fragment_shader_editor->restore_default_help_info();
}

shader_editor_scene::~shader_editor_scene() { persist_sources_to_disk(); }

std::string shader_editor_scene::vertex_path() const {
  return (fs::path(k_project_root) / m_active_project / "vertex.shader")
      .string();
}

std::string shader_editor_scene::fragment_path() const {
  return (fs::path(k_project_root) / m_active_project / "fragment.shader")
      .string();
}

void shader_editor_scene::refresh_project_list() {
  m_project_names.clear();
  std::error_code ec;
  if (!fs::exists(k_project_root, ec) || ec) {
    return;
  }
  for (const auto &entry : fs::directory_iterator(k_project_root, ec)) {
    if (ec) {
      break;
    }
    if (!entry.is_directory()) {
      continue;
    }
    std::string name = entry.path().filename().string();
    if (!name.empty() && name[0] != '.') {
      m_project_names.push_back(std::move(name));
    }
  }
  std::sort(m_project_names.begin(), m_project_names.end());
}

void shader_editor_scene::ensure_at_least_one_project() {
  std::error_code ec;
  fs::create_directories(k_project_root, ec);
  refresh_project_list();
  if (!m_project_names.empty()) {
    return;
  }
  std::string err;
  if (!write_default_shaders_to_dir(fs::path(k_project_root) / k_default_project,
                                    &err)) {
    // best effort; reload will fall back to in-memory defaults
    (void)err;
  }
  refresh_project_list();
}

void shader_editor_scene::switch_to_project(const std::string &name) {
  if (name == m_active_project) {
    return;
  }
  persist_sources_to_disk();
  refresh_project_list();
  if (std::find(m_project_names.begin(), m_project_names.end(), name) ==
      m_project_names.end()) {
    return;
  }
  m_active_project = name;
  reload_sources_from_disk();
  m_vertex_shader_editor->set_text(m_vertex_shader_source);
  m_fragment_shader_editor->set_text(m_fragment_shader_source);
  m_vertex_shader_source = m_vertex_shader_editor->get_text();
  m_fragment_shader_source = m_fragment_shader_editor->get_text();
  std::string err;
  if (!compile_and_replace_shader(&err)) {
    m_vertex_shader_editor->set_help_info(err);
    m_fragment_shader_editor->set_help_info(err);
  } else {
    m_vertex_shader_editor->restore_default_help_info();
    m_fragment_shader_editor->restore_default_help_info();
  }
}

bool shader_editor_scene::try_create_project(const std::string &name,
                                             std::string *error_message) {
  if (!is_valid_project_name(name)) {
    if (error_message) {
      *error_message =
          "Invalid name (use letters, digits, - and _, max 64 chars).";
    }
    return false;
  }
  const fs::path dir = fs::path(k_project_root) / name;
  if (fs::exists(dir)) {
    if (error_message) {
      *error_message = "A project with that name already exists.";
    }
    return false;
  }
  std::string v_src;
  std::string f_src;
  if (m_vertex_shader_editor && m_fragment_shader_editor) {
    v_src = m_vertex_shader_editor->get_text();
    f_src = m_fragment_shader_editor->get_text();
  } else {
    v_src = k_default_vertex_shader;
    f_src = k_default_fragment_shader;
  }
  std::string err;
  if (!write_shaders_to_dir(dir, v_src, f_src, &err)) {
    if (error_message) {
      *error_message = err.empty() ? "Failed to create project." : err;
    }
    return false;
  }
  refresh_project_list();
  switch_to_project(name);
  m_new_project_name[0] = '\0';
  return true;
}

void shader_editor_scene::reload_sources_from_disk() {
  if (!load_text_file(vertex_path().c_str(), m_vertex_shader_source)) {
    m_vertex_shader_source.assign(k_default_vertex_shader);
    (void)save_text_file(vertex_path().c_str(), m_vertex_shader_source);
  }

  if (!load_text_file(fragment_path().c_str(), m_fragment_shader_source)) {
    m_fragment_shader_source.assign(k_default_fragment_shader);
    (void)save_text_file(fragment_path().c_str(), m_fragment_shader_source);
  }
}

void shader_editor_scene::persist_sources_to_disk() const {
  if (!m_vertex_shader_editor || !m_fragment_shader_editor) {
    return;
  }
  const std::string v = m_vertex_shader_editor->get_text();
  const std::string frag = m_fragment_shader_editor->get_text();
  (void)save_text_file(vertex_path().c_str(), v);
  (void)save_text_file(fragment_path().c_str(), frag);
}

void shader_editor_scene::init(GLFWwindow *_window) {
  test_scene_base::init(_window);

  ensure_at_least_one_project();
  refresh_project_list();
  if (m_project_names.empty()) {
    m_active_project = k_default_project;
  } else if (std::find(m_project_names.begin(), m_project_names.end(),
                       m_active_project) == m_project_names.end()) {
    m_active_project = m_project_names.front();
  }

  reload_sources_from_disk();

  m_vertex_shader_editor->set_text(m_vertex_shader_source);
  m_fragment_shader_editor->set_text(m_fragment_shader_source);

  mesh_data data(quad_vertices, sizeof(quad_vertices), quad_indices,
                 sizeof(quad_indices) / sizeof(unsigned int),
                 {{3, GL_FLOAT, GL_FALSE}});
  m_mesh_manager.setup_mesh(data);

  std::string err;
  m_vertex_shader_source = m_vertex_shader_editor->get_text();
  m_fragment_shader_source = m_fragment_shader_editor->get_text();
  if (!compile_and_replace_shader(&err)) {
    m_vertex_shader_editor->set_help_info(err);
    m_fragment_shader_editor->set_help_info(err);
  }
}

void shader_editor_scene::render() {
  if (!m_shader) {
    return;
  }
  m_shader->use();
  update_builtin_uniforms(m_window);
  apply_builtin_uniforms(m_shader);
  m_mesh_manager.draw();
}

void shader_editor_scene::render_ui() {
  ImGui::SeparatorText("Shader project (subfolder)");
  refresh_project_list();

  const char *preview =
      m_active_project.empty() ? "(none)" : m_active_project.c_str();
  if (ImGui::BeginCombo("Project##shader_proj", preview)) {
    for (const auto &name : m_project_names) {
      bool selected = (name == m_active_project);
      if (ImGui::Selectable(name.c_str(), selected)) {
        switch_to_project(name);
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::SameLine();
  if (ImGui::Button("Refresh##shader_proj")) {
    refresh_project_list();
  }

  ImGui::InputTextWithHint("##new_shader_proj", "new-project-name",
                           m_new_project_name, sizeof(m_new_project_name));
  ImGui::SameLine();
  if (ImGui::Button("Create project")) {
    std::string err;
    if (!try_create_project(std::string(m_new_project_name), &err)) {
      m_create_project_error = std::move(err);
      ImGui::OpenPopup("Shader project error");
    }
  }
  if (ImGui::BeginPopupModal("Shader project error", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextWrapped("%s", m_create_project_error.c_str());
    if (ImGui::Button("OK")) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  if (!m_show_vertex_shader_editor) {
    if (ImGui::Button("Show Vertex Shader Editor")) {
      m_show_vertex_shader_editor = true;
    }
  } else {
    m_vertex_shader_editor->render();
  }

  if (!m_show_fragment_shader_editor) {
    if (ImGui::Button("Show Fragment Shader Editor")) {
      m_show_fragment_shader_editor = true;
    }
  } else {
    m_fragment_shader_editor->render();
  }

  ImGui::SeparatorText("User builtin variables");
  ui_builtin_uniforms();
}

bool shader_editor_scene::compile_and_replace_shader(
    std::string *_error_message) {
  try {
    shader *new_shader = shader::shader_from_source(m_vertex_shader_source,
                                                    m_fragment_shader_source);
    if (new_shader) {
      if (m_shader) {
        delete m_shader;
      }
      m_shader = new_shader;
      return true;
    } else {
      return false;
    }
  } catch (std::exception &e) {
    if (_error_message) {
      *_error_message = e.what();
    }
    return false;
  }
}

bool shader_editor_scene::on_save_shader() {
  m_vertex_shader_source = m_vertex_shader_editor->get_text();
  m_fragment_shader_source = m_fragment_shader_editor->get_text();
  persist_sources_to_disk();
  std::string error_message;
  if (!compile_and_replace_shader(&error_message)) {
    if (m_show_vertex_shader_editor) {
      m_vertex_shader_editor->set_help_info(error_message);
    }
    if (m_show_fragment_shader_editor) {
      m_fragment_shader_editor->set_help_info(error_message);
    }
  } else {
    m_vertex_shader_editor->restore_default_help_info();
    m_fragment_shader_editor->restore_default_help_info();
  }
  return true;
}
