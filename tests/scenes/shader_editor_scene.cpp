#include "shader_editor_scene.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include <algorithm>
#include <cctype>

static float quad_vertices[] = {-1.0f, -1.0f, 0.0f, 1.0f,  -1.0f, 0.0f,
                                1.0f,  1.0f,  0.0f, -1.0f, 1.0f,  0.0f};

static unsigned int quad_indices[] = {0, 1, 2, 2, 3, 0};

shader_editor_scene::shader_editor_scene() : test_scene_base("Shader Editor") {
  m_vertex_shader_editor = std::make_unique<shader_editor>("Vertex Shader");
  m_fragment_shader_editor = std::make_unique<shader_editor>("Fragment Shader");
  m_vertex_shader_editor->set_save_callback(
      [this]() { return on_save_shader(); });
  m_fragment_shader_editor->set_save_callback(
      [this]() { return on_save_shader(); });
}

shader_editor_scene::~shader_editor_scene() {}

void shader_editor_scene::init(GLFWwindow *_window) {
  test_scene_base::init(_window);

  m_vertex_shader_editor->set_text(m_vertex_shader_source);
  m_fragment_shader_editor->set_text(m_fragment_shader_source);

  mesh_data data(quad_vertices, sizeof(quad_vertices), quad_indices,
                 sizeof(quad_indices) / sizeof(unsigned int),
                 {{3, GL_FLOAT, GL_FALSE}});
  m_mesh_manager.setup_mesh(data);
}

void shader_editor_scene::render() {
  if (!m_shader) {
    return;
  }
  m_shader->use();
  m_mesh_manager.draw();
}

void shader_editor_scene::render_ui() {
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