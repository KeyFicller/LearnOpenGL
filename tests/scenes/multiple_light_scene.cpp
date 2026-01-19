#include "multiple_light_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_cube.h"
#include "tests/component/shader_loader.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

multiple_light_scene::multiple_light_scene()
    : renderable_scene_base("Multiple Light Test") {}

void multiple_light_scene::init(GLFWwindow *_window) {
  renderable_scene_base::init(_window);

  // Setup mesh using cube helper
  prefab_cube::cube_mesh_data cube_data(
      prefab_cube::vertex_format::position_normal);
  setup_mesh(*cube_data.mesh);

  // Load shaders using helper
  load_shader_pair("shaders/multiple_light_test/vertex.shader",
                   "shaders/multiple_light_test/fragment.shader",
                   "shaders/multiple_light_test/light_fragment.shader",
                   m_shader, m_light_shader);

  // Initialize camera - position at (10, 10, 10) looking at origin
  m_camera.m_position = {10.0f, 10.0f, 10.0f};
  m_camera.m_yaw = -135.0f; // Looking from (10,10,10) towards (0,0,0)
  m_camera.m_pitch = -35.26f;
  m_camera.update_view_matrix();

  for (int i = 0; i < 4; i++) {
    m_lights[i].Position = {0.0f, 0.0f, (float)i};
    m_lights[i].Type = light_type::k_point;
  }
}

void multiple_light_scene::render() {
  if (!m_shader) {
    return;
  }

  // Set matrices and render object
  set_matrices(m_shader);
  for (int i = 0; i < 4; i++) {
    std::string light_name = "uLight[" + std::to_string(i) + "]";
    uniform(*m_shader, m_lights[i], light_name);
  }
  uniform(*m_shader, m_material, "uMaterial");
  m_shader->set_uniform<glm::vec3, 1>("uEyePosition", &m_camera.m_position);
  m_mesh.draw();

  // Render light source using helper
  for (int i = 0; i < 4; i++) {
    render_light_source(m_light_shader, m_lights[i].Position);
  }
}

void multiple_light_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Multiple Light Test");
  ImGui::Spacing();

  render_camera_ui();
  ImGui::PushID("Light");
  ImGui::SliderInt("Editing Light", &m_editing_light, 0, 3);
  ImGui::Text("Light %d", m_editing_light);
  ui(m_lights[m_editing_light]);
  ImGui::PopID();

  // ImGui::Separator();
  // ImGui::Text("Material");
  // ui(m_material);
  // ImGui::Separator();
}
