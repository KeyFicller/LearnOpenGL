#include "shader_editor_scene.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "tests/component/mesh_manager.h"

static float quad_vertices[] = {-1.0f, -1.0f, 0.0f, 1.0f,  -1.0f, 0.0f,
                                1.0f,  1.0f,  0.0f, -1.0f, 1.0f,  0.0f};

static unsigned int quad_indices[] = {0, 1, 2, 2, 3, 0};

shader_editor_scene::shader_editor_scene() : test_scene_base("Shader Editor") {}

shader_editor_scene::~shader_editor_scene() {}

void shader_editor_scene::init(GLFWwindow *_window) {
  test_scene_base::init(_window);

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
  ImGui::Begin("Fragment Shader Editor");

  ImVec2 content_region_avail = ImGui::GetContentRegionAvail();
  ImGui::InputTextMultiline("##Fragment Shader", &m_fragment_shader_source,
                            content_region_avail,
                            ImGuiInputTextFlags_AllowTabInput);

  ImGui::End();

  if (ImGui::Button("Compile and Replace Shader")) {
    if (!compile_and_replace_shader()) {
      ImGui::BeginPopupModal("Compile Error", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize);
      ImGui::Text("Failed to compile shader");
      ImGui::EndPopup();
    }
  }
}

bool shader_editor_scene::compile_and_replace_shader() {
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
  } catch (const std::exception &e) {
    return false;
  }
}