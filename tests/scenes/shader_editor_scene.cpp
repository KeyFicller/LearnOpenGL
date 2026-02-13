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

  if (!m_is_editing_fragment_shader) {
    if (ImGui::Button("Edit Fragment Shader")) {
      m_is_editing_fragment_shader = true;
      m_editor.SetText(m_fragment_shader_source);
    }
  } else {
    auto cpos = m_editor.GetCursorPosition();
    ImGui::Begin("Fragment Shader Editor");
    ImGui::TextDisabled(m_edit_hint.c_str());
    // ImVec2 content_region_avail = ImGui::GetContentRegionAvail();
    // ImGui::InputTextMultiline("##Fragment Shader", &m_fragment_shader_source,
    //                           content_region_avail,
    //                           ImGuiInputTextFlags_AllowTabInput);

    ImGui::Text(
        "%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1,
        cpos.mColumn + 1, m_editor.GetTotalLines(),
        m_editor.IsOverwrite() ? "Ovr" : "Ins", m_editor.CanUndo() ? "*" : " ",
        m_editor.GetLanguageDefinition().mName.c_str(), "Fragment Shader");

    m_editor.Render("Test");
    if (m_editor.IsTextChanged()) {
      // Restore Hint
      m_edit_hint = "Ctrl+S to save";
    }

    ImGuiIO &io = ImGui::GetIO();
    bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
      m_fragment_shader_source = m_editor.GetText();
      std::string error_message;
      if (!compile_and_replace_shader(&error_message)) {
        m_edit_hint = error_message;
      } else {
        m_is_editing_fragment_shader = false;
      }
    }

    ImGui::End();
  }

  if (ImGui::Button("Compile and Replace Shader")) {
    if (!compile_and_replace_shader()) {
      ImGui::BeginPopupModal("Compile Error", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize);
      ImGui::Text("Failed to compile shader");
      ImGui::EndPopup();
    }
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