#include "tessellation_shader_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include <GLFW/glfw3.h>

tessellation_shader_scene::tessellation_shader_scene()
    : renderable_scene_base("Tessellation Shader") {}

tessellation_shader_scene::~tessellation_shader_scene() { delete m_shader; }

void tessellation_shader_scene::init(GLFWwindow *_window) {
  renderable_scene_base::init(_window);

  m_camera.Position = {0.0f, 0.0f, 3.0f};
  m_camera.Yaw = -90.0f;
  m_camera.Pitch = 0.0f;
  m_camera.Orthographic = true;
  m_camera.update_view_matrix();

  static float patch_vertices[] = {
      -0.8f, -0.8f, 0.0f, 0.8f, -0.8f, 0.0f, 0.0f, 0.8f, 0.0f,
  };

  mesh_data patch_data(patch_vertices, sizeof(patch_vertices), 3,
                       {vertex_attribute{3, GL_FLOAT, false}});
  m_patch_mesh.setup_mesh(patch_data);

  try {
    m_shader =
        new shader("shaders/tessellation_shader_test/vertex.shader",
                   "shaders/tessellation_shader_test/fragment.shader", nullptr,
                   "shaders/tessellation_shader_test/tess_control.shader",
                   "shaders/tessellation_shader_test/tess_eval.shader");
  } catch (const std::exception &e) {
    m_supported = false;
    m_error_message = e.what();
  }
}

void tessellation_shader_scene::render() {
  if (!m_supported || !m_shader) {
    return;
  }

  m_shader->use();
  set_matrices(m_shader);
  m_shader->set_uniform("uOuterLevel", m_outer_level);
  m_shader->set_uniform("uInnerLevel", m_inner_level);
  m_shader->set_uniform("uAmplitude", m_amplitude);
  m_shader->set_uniform("uTime", static_cast<float>(glfwGetTime()));

  m_patch_mesh.bind();
  glDrawArrays(GL_PATCHES, 0,
               static_cast<GLsizei>(m_patch_mesh.get_index_count()));
}

void tessellation_shader_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Tessellation Shader");
  ImGui::Spacing();

  if (!m_supported || !m_shader) {
    ImGui::TextWrapped(
        "Tessellation shader is unavailable on current context.");
    if (!m_error_message.empty()) {
      ImGui::TextWrapped("%s", m_error_message.c_str());
    }
    return;
  }

  render_camera_ui();
  ImGui::SliderFloat("Outer Tess Level", &m_outer_level, 1.0f, 16.0f);
  ImGui::SliderFloat("Inner Tess Level", &m_inner_level, 1.0f, 16.0f);
  ImGui::SliderFloat("Wave Amplitude", &m_amplitude, 0.0f, 0.5f);
}
