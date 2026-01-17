#include "stencil_test_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_cube.h"
#include "tests/component/shader_loader.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

stencil_test_scene::stencil_test_scene()
    : renderable_scene_base("Stencil Test") {}

void stencil_test_scene::init(GLFWwindow *_window) {
  renderable_scene_base::init(_window);

  // Enable stencil test
  glEnable(GL_STENCIL_TEST);

  // Setup mesh using cube helper
  prefab_cube::cube_mesh_data cube_data(
      prefab_cube::vertex_format::position_normal);
  setup_mesh(cube_data.mesh);

  // Load shaders using helper
  load_shader_pair("shaders/stencil_test_test/vertex.shader",
                   "shaders/stencil_test_test/fragment.shader",
                   "shaders/stencil_test_test/light_fragment.shader", m_shader,
                   m_light_shader);

  // Initialize camera - position at (10, 10, 10) looking at origin
  m_camera.m_position = {10.0f, 10.0f, 10.0f};
  m_camera.m_yaw = -135.0f; // Looking from (10,10,10) towards (0,0,0)
  m_camera.m_pitch = -35.26f;
  m_camera.update_view_matrix();
}

void stencil_test_scene::render() {
  if (!m_shader) {
    return;
  }

  // Configure stencil test for writing
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilMask(0xFF);

  // Draw normal object and write to stencil buffer
  m_shader->use();
  set_matrices(m_shader, glm::translate(glm::mat4(1.0f), m_object_position));
  m_shader->set_uniform<float, 3>("uLightColor", &m_light_color.x);
  m_shader->set_uniform<float, 3>("uObjectColor", &m_object_color.x);
  m_shader->set_uniform<float, 1>("uAmbientStrength", &m_ambient_strength);
  m_shader->set_uniform<float, 3>("uLightPosition", &m_light_position.x);
  m_shader->set_uniform<float, 1>("uSpecularStrength", &m_specular_strength);
  m_shader->set_uniform<float, 3>("uEyePosition", &m_camera.m_position.x);
  m_shader->set_uniform<float, 1>("uShininess", &m_shininess);
  m_shader->set_uniform("uDrawBoundaries", 0); // false = 0
  m_mesh.draw();

  // Draw boundary outline using stencil test
  glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
  glStencilMask(0x00); // Disable writing to stencil buffer
  glDisable(GL_DEPTH_TEST);

  set_matrices(m_shader,
               glm::translate(glm::mat4(1.0f), m_object_position) *
                   glm::scale(glm::mat4(1.0f), glm::vec3(m_boundary_scale)));
  m_shader->set_uniform("uDrawBoundaries", 1); // true = 1
  m_shader->set_uniform("uBoundaryColor", m_boundary_color);
  m_mesh.draw();

  glEnable(GL_DEPTH_TEST);
  glStencilMask(0xFF); // Re-enable stencil writing for next frame

  // Render light source using helper
  render_light_source(m_light_shader, m_light_position);
}

void stencil_test_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Stencil Test");
  ImGui::Spacing();

  render_camera_ui();

  // ImGui::Checkbox("Stencil Test", &m_stencil_test);
  // ImGui::Combo("Stencil Test Mode", &m_stencil_test_index,
  //              stencil_test_mode_names,
  //              IM_ARRAYSIZE(stencil_test_mode_names));
  ImGui::SliderFloat("Boundary Scale", &m_boundary_scale, 1.0f, 1.2f, "%.3f");
  ImGui::ColorEdit3("Boundary Color", &m_boundary_color.x);
  ImGui::SliderFloat3("Object Position", &m_object_position.x, -10.0f, 10.0f,
                      "%.2f");
}
