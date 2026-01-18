#include "light_type_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_cube.h"
#include "tests/component/shader_loader.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

light_type_scene::light_type_scene()
    : renderable_scene_base("Light Type Test") {}

void light_type_scene::init(GLFWwindow *_window) {
  renderable_scene_base::init(_window);

  // Setup mesh using cube helper
  prefab_cube::cube_mesh_data cube_data(
      prefab_cube::vertex_format::position_normal);
  setup_mesh(cube_data.mesh);

  // Load shaders using helper
  load_shader_pair("shaders/light_type_test/vertex.shader",
                   "shaders/light_type_test/fragment.shader",
                   "shaders/light_type_test/light_fragment.shader", m_shader,
                   m_light_shader);

  // Initialize camera - position at (10, 10, 10) looking at origin
  m_camera.m_position = {10.0f, 10.0f, 10.0f};
  m_camera.m_yaw = -135.0f; // Looking from (10,10,10) towards (0,0,0)
  m_camera.m_pitch = -35.26f;
  m_camera.update_view_matrix();
}

void light_type_scene::render() {
  if (!m_shader) {
    return;
  }

  // Set matrices and render object
  set_matrices(m_shader);
  uniform(*m_shader, m_light, "uLight");
  uniform(*m_shader, m_material, "uMaterial");
  m_shader->set_uniform<glm::vec3, 1>("uEyePosition", &m_camera.m_position);
  m_mesh.draw();

  // Render light source using helper
  render_light_source(m_light_shader, m_light.Position);
}

void light_type_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Light Type Test");
  ImGui::Spacing();

  render_camera_ui();
  ImGui::PushID("Light");
  ImGui::Text("Light");
  ui(m_light);
  ImGui::PopID();
  ImGui::PushID("Material");
  ImGui::Text("Material");
  ui(m_material);
  ImGui::PopID();

  ImGui::Separator();
}
