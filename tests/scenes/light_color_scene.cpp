#include "light_color_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_cube.h"
#include "tests/component/shader_loader.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

light_color_scene::light_color_scene()
    : renderable_scene_base("Light Color Test") {}

void light_color_scene::init(GLFWwindow *_window) {
  renderable_scene_base::init(_window);

  // Setup mesh using cube helper
  prefab_cube::cube_mesh_data cube_data(
      prefab_cube::vertex_format::position_normal);
  setup_mesh(*cube_data.mesh);

  // Load shaders using helper
  load_shader_pair("shaders/light_color_test/vertex.shader",
                   "shaders/light_color_test/fragment.shader",
                   "shaders/light_color_test/light_fragment.shader", m_shader,
                   m_light_shader);

  // Initialize camera - position at (10, 10, 10) looking at origin
  m_camera.Position = {10.0f, 10.0f, 10.0f};
  m_camera.Yaw = -135.0f; // Looking from (10,10,10) towards (0,0,0)
  m_camera.Pitch = -35.26f;
  m_camera.update_view_matrix();
}

void light_color_scene::render() {
  if (!m_shader) {
    return;
  }

  // Set matrices and render object
  set_matrices(m_shader);
  m_shader->set_uniform("uLightColor", m_light_color);
  m_shader->set_uniform("uObjectColor", m_object_color);
  m_shader->set_uniform("uAmbientStrength", m_ambient_strength);
  m_shader->set_uniform("uLightPosition", m_light_position);
  m_shader->set_uniform("uSpecularStrength", m_specular_strength);
  m_shader->set_uniform("uEyePosition", m_camera.Position);
  m_shader->set_uniform("uShininess", m_shininess);
  m_mesh.draw();

  // Render light source using helper
  render_light_source(m_light_shader, m_light_position);
}

void light_color_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Light Color Test");
  ImGui::Spacing();

  render_camera_ui();
  ImGui::SliderFloat3("Light Color", &m_light_color.x, 0.0f, 1.0f);
  ImGui::SliderFloat3("Object Color", &m_object_color.x, 0.0f, 1.0f);
  ImGui::SliderFloat3("Light Position", &m_light_position.x, -10.0f, 10.0f);

  ImGui::Separator();
  ImGui::SliderFloat("Ambient Strength", &m_ambient_strength, 0.0f, 1.0f,
                     "%.2f");
  ImGui::SliderFloat("Specular Strength", &m_specular_strength, 0.0f, 1.0f,
                     "%.2f");
  ImGui::SliderFloat("Shininess", &m_shininess, 1.0f, 128.0f, "%.0f");
}
