#include "instance_scene.h"

#include "basic/shader.h"
#include "glad/gl.h"
#include "glm/ext/matrix_transform.hpp"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_quad.h"
#include "tests/component/shader_loader.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>

instance_scene::instance_scene() : renderable_scene_base("Instance Scene") {}

void instance_scene::init(GLFWwindow *window) {
  renderable_scene_base::init(window);

  // Setup mesh using quad helper
  prefab_quad::quad_mesh_data quad_data(
      prefab_quad::vertex_format::position_normal);
  setup_mesh(*quad_data.mesh);

  // Initialize camera
  m_camera.m_position = {0.0f, 0.0f, 3.0f};
  m_camera.m_yaw = -90.0f; // Default looking along -Z axis
  m_camera.m_pitch = 0.0f;
  m_camera.update_view_matrix();

  // Add sub-scenes
  m_sub_scenes.add_sub_scene(
      std::make_unique<instance_uniform_pass_value_scene>(this));
  m_sub_scenes.add_sub_scene(
      std::make_unique<instance_attrib_pass_value_scene>(this));
}

void instance_scene::render() { m_sub_scenes.render(); }

void instance_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Instance Scene");
  ImGui::Spacing();

  render_camera_ui();

  // Sub-scene selection UI
  auto names = m_sub_scenes.get_names();
  int current_index = m_sub_scenes.current_index();
  if (ImGui::Combo("Scene Mode", &current_index, names.data(),
                   static_cast<int>(names.size()))) {
    m_sub_scenes.set_current(current_index);
  }

  // Render current sub-scene UI
  m_sub_scenes.render_ui();

  ImGui::Separator();
  ImGui::Text("Instance count");
  ImGui::SliderInt("Row count", &m_row, 1, 10);
  ImGui::SliderInt("Column count", &m_col, 1, 10);
}