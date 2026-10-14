#include "shader_editor_scene.h"

#include "imgui.h"

#include "tests/component/mesh_manager.h"
#include "tests/component/shader_editor_chat.h"
#include "tests/component/shader_editor_mcp.h"
#include "tests/framework/test_suit.h"

#include "basic/texture.h"

#include <GLFW/glfw3.h>
#include <glad/gl.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <array>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <iterator>
#include <nfd.h>
#include <random>
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

void squeeze_trailing_blank_newlines(std::string &buffer) {
  while (buffer.size() >= 2 && buffer.back() == '\n' &&
         buffer[buffer.size() - 2] == '\n') {
    buffer.pop_back();
  }
}

/* Formats loadable by stb_image (see basic/texture.cpp). Comma-separated specs
 * per nativefiledialog-extended (no leading dot). */
static const nfdfilteritem_t k_builtin_texture_file_filters[] = {
    {"All supported",
     "png,jpg,jpeg,jpe,jfif,bmp,dib,tga,gif,hdr,pic,pnm,ppm,pgm,pbm,psd"},
    {"PNG", "png"},
    {"JPEG", "jpg,jpeg,jpe,jfif"},
    {"BMP / DIB", "bmp,dib"},
    {"TGA", "tga"},
    {"GIF", "gif"},
    {"Radiance HDR", "hdr"},
    {"Softimage PIC", "pic"},
    {"PNM / PPM / PGM", "pnm,ppm,pgm,pbm"},
    {"Photoshop PSD", "psd"},
};

struct builtin_uniforms {
  float u_time = 0.0f;
  float u_random = 0.0f;
  glm::vec2 u_mouse = glm::vec2(0.0f, 0.0f);
  glm::vec2 u_resolution = glm::vec2(0.0f, 0.0f);
};

static builtin_uniforms _builtin_uniforms;

void update_builtin_uniforms(GLFWwindow *window) {
  _builtin_uniforms.u_time = static_cast<float>(glfwGetTime());
  thread_local bool rng_seeded = false;
  thread_local std::mt19937 rng{};
  if (!rng_seeded) {
    rng.seed(std::random_device{}());
    rng_seeded = true;
  }
  std::uniform_real_distribution<float> u01(0.0f, 1.0f);
  _builtin_uniforms.u_random = u01(rng);
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
  _shader->set_uniform<float>("u_random", _builtin_uniforms.u_random);
  _shader->set_uniform<glm::vec2>("u_mouse", _builtin_uniforms.u_mouse);
  _shader->set_uniform<glm::vec2>("u_resolution",
                                  _builtin_uniforms.u_resolution);
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
  m_chat = std::make_unique<shader_editor_chat>();
}

shader_editor_scene::~shader_editor_scene() {
  // The chat process calls back into the MCP server, so it has to be gone
  // before that server is.
  m_chat.reset();
  // Stop the server before the editors it calls into are gone. An in-flight
  // tool waits at most a few seconds for a frame that will not come.
  m_mcp.reset();
  persist_sources_to_disk();
}

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
  } else {
    squeeze_trailing_blank_newlines(m_vertex_shader_source);
  }

  if (!load_text_file(fragment_path().c_str(), m_fragment_shader_source)) {
    m_fragment_shader_source.assign(k_default_fragment_shader);
    (void)save_text_file(fragment_path().c_str(), m_fragment_shader_source);
  } else {
    squeeze_trailing_blank_newlines(m_fragment_shader_source);
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

  {
    std::error_code ec_png;
    fs::create_directories(fs::path(k_project_root) / k_user_png_subdir,
                         ec_png);
    const std::array<uint8_t, 4> white{{255, 255, 255, 255}};
    m_builtin_user_png = std::make_unique<texture_2d>(white);
    m_builtin_png_path_label =
        "Default 1x1 white (click thumbnail — pick image under "
        + std::string(k_project_root) + "/" + k_user_png_subdir + ")";
    m_builtin_png_load_error.clear();
  }

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

  m_mcp = std::make_unique<shader_editor_mcp>(this);
  m_mcp->start();
}

void shader_editor_scene::render() {
  drain_mcp_tasks();
  // Runs on every exit, including the early one, so a preview waits for this
  // frame's image rather than the cleared buffer of the next.
  struct capture_guard {
    shader_editor_scene *self;
    ~capture_guard() { self->finish_capture(); }
  } guard{this};

  if (!m_shader) {
    return;
  }
  m_shader->use();
  update_builtin_uniforms(m_window);
  constexpr int k_builtin_tex_unit = 0;
  if (m_builtin_user_png) {
    m_builtin_user_png->bind(k_builtin_tex_unit);
  }
  m_shader->set_uniform<int>("u_texture", k_builtin_tex_unit);
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

  draw_builtin_uniforms_panel();
  if (m_chat) {
    m_chat->draw();
  }
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

bool shader_editor_scene::try_reload_builtin_user_png(
    const std::string &_path_utf8) {
  m_builtin_png_load_error.clear();
  try {
    auto loaded = std::make_unique<texture_2d>(_path_utf8.c_str());
    m_builtin_user_png = std::move(loaded);
    m_builtin_png_path_label = _path_utf8;
    return true;
  } catch (const std::exception &e) {
    m_builtin_png_load_error = e.what();
    return false;
  }
}

void shader_editor_scene::open_user_texture_file_dialog() {
  std::error_code ec;
  const fs::path folder = fs::path(k_project_root) / k_user_png_subdir;
  fs::create_directories(folder, ec);
  std::string default_path = fs::absolute(folder).string();

  nfdchar_t *out_path = nullptr;
  constexpr nfdfiltersize_t n_filters =
      sizeof(k_builtin_texture_file_filters) /
      sizeof(k_builtin_texture_file_filters[0]);
  nfdresult_t r = NFD_OpenDialog(&out_path, k_builtin_texture_file_filters,
                                  n_filters, default_path.c_str());
  if (r == NFD_OKAY && out_path) {
    (void)try_reload_builtin_user_png(std::string(out_path));
    NFD_FreePath(out_path);
  } else if (r == NFD_ERROR && NFD_GetError() != nullptr) {
    m_builtin_png_load_error = NFD_GetError();
  }
}

void shader_editor_scene::draw_builtin_uniforms_panel() {
  const int port = mcp_port();
  if (port > 0) {
    ImGui::Text("MCP  http://127.0.0.1:%d/mcp", port);
  }
  ImGui::SeparatorText("User builtin variables");
  ImGui::Text("u_time: %.2f", _builtin_uniforms.u_time);
  ImGui::Text("u_random: %.6f  (uniform in [0,1), new each frame)",
              _builtin_uniforms.u_random);
  ImGui::Text("u_resolution: %.0f x %.0f", _builtin_uniforms.u_resolution.x,
              _builtin_uniforms.u_resolution.y);
  ImGui::Text("u_mouse (norm, GL Y-up): (%.3f, %.3f)",
              _builtin_uniforms.u_mouse.x, _builtin_uniforms.u_mouse.y);

  ImGui::Separator();
  ImGui::TextWrapped("uniform sampler2D u_texture  (texture unit %d)", 0);
  constexpr ImVec2 thumb(112.0f, 112.0f);
  if (m_builtin_user_png) {
    ImGui::Image(
        reinterpret_cast<void *>(
            static_cast<intptr_t>(m_builtin_user_png->ID())),
        thumb, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
      open_user_texture_file_dialog();
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip(
          "Click — system file picker.\n"
          "PNG, JPEG, BMP, TGA, GIF, HDR, PIC, PNM, PSD (stb_image).\n"
          "Starts in %s/%s",
          k_project_root, k_user_png_subdir);
    }
  }
  ImGui::SameLine();
  ImGui::BeginGroup();
  if (m_builtin_user_png) {
    ImGui::Text("Dimensions: %d x %d", m_builtin_user_png->width(),
                m_builtin_user_png->height());
  }
  ImGui::TextWrapped("%s", m_builtin_png_path_label.c_str());
  if (!m_builtin_png_load_error.empty()) {
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 90, 90, 255));
    ImGui::TextWrapped("%s", m_builtin_png_load_error.c_str());
    ImGui::PopStyleColor();
    if (ImGui::SmallButton("Clear error")) {
      m_builtin_png_load_error.clear();
    }
  }
  ImGui::EndGroup();
}

std::string shader_editor_scene::vertex_text() const {
  return m_vertex_shader_editor ? m_vertex_shader_editor->get_text()
                                : m_vertex_shader_source;
}

std::string shader_editor_scene::fragment_text() const {
  return m_fragment_shader_editor ? m_fragment_shader_editor->get_text()
                                  : m_fragment_shader_source;
}

void shader_editor_scene::refresh_projects() { refresh_project_list(); }

bool shader_editor_scene::apply_shader(const std::string &vertex,
                                       const std::string &fragment,
                                       std::string *error) {
  if (!vertex.empty()) {
    m_vertex_shader_editor->set_text(vertex);
  }
  if (!fragment.empty()) {
    m_fragment_shader_editor->set_text(fragment);
  }
  m_show_vertex_shader_editor = true;
  m_show_fragment_shader_editor = true;
  m_vertex_shader_source = m_vertex_shader_editor->get_text();
  m_fragment_shader_source = m_fragment_shader_editor->get_text();
  persist_sources_to_disk();
  std::string err;
  if (!compile_and_replace_shader(&err)) {
    m_vertex_shader_editor->set_help_info(err);
    m_fragment_shader_editor->set_help_info(err);
    if (error) {
      *error = std::move(err);
    }
    return false;
  }
  m_vertex_shader_editor->restore_default_help_info();
  m_fragment_shader_editor->restore_default_help_info();
  if (error) {
    error->clear();
  }
  return true;
}

bool shader_editor_scene::open_project(const std::string &name,
                                       std::string *error) {
  refresh_project_list();
  if (std::find(m_project_names.begin(), m_project_names.end(), name) ==
      m_project_names.end()) {
    if (error) {
      *error = "No shader project named \"" + name + "\".";
    }
    return false;
  }
  switch_to_project(name);
  if (error) {
    error->clear();
  }
  return true;
}

shader_editor_scene::builtin_uniform_sample
shader_editor_scene::sample_builtin_uniforms() {
  update_builtin_uniforms(m_window);
  builtin_uniform_sample sample;
  sample.time = _builtin_uniforms.u_time;
  sample.random = _builtin_uniforms.u_random;
  sample.mouse_x = _builtin_uniforms.u_mouse.x;
  sample.mouse_y = _builtin_uniforms.u_mouse.y;
  sample.resolution_x = _builtin_uniforms.u_resolution.x;
  sample.resolution_y = _builtin_uniforms.u_resolution.y;
  sample.texture_label = m_builtin_png_path_label;
  if (m_builtin_user_png) {
    sample.texture_w = m_builtin_user_png->width();
    sample.texture_h = m_builtin_user_png->height();
  }
  return sample;
}

void shader_editor_scene::drain_mcp_tasks() {
  std::vector<std::function<void()>> tasks;
  {
    std::lock_guard<std::mutex> lock(m_mcp_mu);
    tasks.swap(m_mcp_tasks);
  }
  for (auto &task : tasks) {
    task();
  }
}

shader_editor_scene::preview shader_editor_scene::read_framebuffer() {
  preview shot;
  test_suit *ts =
      m_window ? static_cast<test_suit *>(glfwGetWindowUserPointer(m_window))
               : nullptr;
  framebuffer *fb = ts ? ts->m_scene_framebuffer : nullptr;
  if (!fb || fb->get_width() <= 0 || fb->get_height() <= 0) {
    shot.error = "The scene framebuffer is not ready.";
    return shot;
  }
  const int w = fb->get_width();
  const int h = fb->get_height();
  std::vector<unsigned char> rgb(static_cast<size_t>(w) * h * 3);
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
  if (const GLenum gl_error = glGetError(); gl_error != GL_NO_ERROR) {
    shot.error = "glReadPixels failed: " + std::to_string(gl_error);
    return shot;
  }

  unsigned long sum_r = 0, sum_g = 0, sum_b = 0;
  std::string grid;
  for (int gy = 0; gy < 8; ++gy) {
    if (gy != 0) {
      grid.push_back('\n');
    }
    const int y0 = h - (gy + 1) * h / 8;
    const int y1 = h - gy * h / 8;
    for (int gx = 0; gx < 8; ++gx) {
      const int x0 = gx * w / 8;
      const int x1 = (gx + 1) * w / 8;
      unsigned long cr = 0, cg = 0, cb = 0, n = 0;
      for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
          const unsigned char *p = rgb.data() + (static_cast<size_t>(y) * w + x) * 3;
          cr += p[0];
          cg += p[1];
          cb += p[2];
          ++n;
        }
      }
      if (n == 0) {
        continue;
      }
      sum_r += cr / n;
      sum_g += cg / n;
      sum_b += cb / n;
      char hex[8];
      std::snprintf(hex, sizeof(hex), "#%02x%02x%02x",
                    static_cast<unsigned>(cr / n), static_cast<unsigned>(cg / n),
                    static_cast<unsigned>(cb / n));
      if (gx != 0) {
        grid.push_back(' ');
      }
      grid += hex;
    }
  }

  const fs::path path = fs::absolute("runtime/shader_preview.png");
  std::error_code ec;
  fs::create_directories(path.parent_path(), ec);
  stbi_flip_vertically_on_write(1);
  if (stbi_write_png(path.string().c_str(), w, h, 3, rgb.data(), w * 3) == 0) {
    shot.error = "Failed to write " + path.string();
    return shot;
  }
  shot.ok = true;
  shot.width = w;
  shot.height = h;
  shot.path = path.string();
  shot.mean_rgb = std::to_string(sum_r / 64) + ", " + std::to_string(sum_g / 64) +
                  ", " + std::to_string(sum_b / 64);
  shot.grid = std::move(grid);
  return shot;
}

void shader_editor_scene::finish_capture() {
  std::shared_ptr<std::promise<preview>> pending;
  {
    std::lock_guard<std::mutex> lock(m_mcp_mu);
    pending = std::move(m_capture);
  }
  if (!pending) {
    return;
  }
  try {
    pending->set_value(read_framebuffer());
  } catch (...) {
    pending->set_exception(std::current_exception());
  }
}

shader_editor_scene::preview shader_editor_scene::capture_preview() {
  auto prom = std::make_shared<std::promise<preview>>();
  {
    std::lock_guard<std::mutex> lock(m_mcp_mu);
    m_capture = prom;
  }
  auto fut = prom->get_future();
  if (fut.wait_for(std::chrono::seconds(3)) == std::future_status::ready) {
    return fut.get();
  }
  {
    std::lock_guard<std::mutex> lock(m_mcp_mu);
    if (m_capture == prom) {
      m_capture.reset();
    }
  }
  if (fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    return fut.get();
  }
  preview shot;
  shot.error =
      "Shader Editor scene is not rendering. Select it in Scene Selection.";
  return shot;
}

int shader_editor_scene::mcp_port() const {
  return m_mcp ? m_mcp->http_port() : -1;
}

