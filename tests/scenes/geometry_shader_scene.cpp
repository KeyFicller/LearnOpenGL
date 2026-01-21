#include "geometry_shader_scene.h"

#include "basic/vertex_array.h"
#include "geometry_shader_sub_scenes.h"
#include "glad/gl.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_cube.h"
#include "tests/component/prefab_quad.h"
#include "tests/component/shader_loader.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

geometry_shader_scene::geometry_shader_scene()
    : renderable_scene_base("Geometry Shader") {}

void geometry_shader_scene::init(GLFWwindow *_window) {
  renderable_scene_base::init(_window);

  // Initialize camera
  m_camera.m_position = {0.0f, 0.0f, 3.0f};
  m_camera.m_yaw = -90.0f;
  m_camera.m_pitch = 0.0f;
  m_camera.m_orthographic = true;
  m_camera.update_view_matrix();

  // Initialize sub-scenes
  m_sub_scene_manager.add_sub_scene(
      std::make_unique<geometry_shader_line_strip_scene>(this));

  m_sub_scene_manager.add_sub_scene(
      std::make_unique<geometry_shader_triangle_strip_scene>(this));

  m_sub_scene_manager.add_sub_scene(
      std::make_unique<geometry_shader_explosion_scene>(this));

  m_sub_scene_manager.add_sub_scene(
      std::make_unique<geometry_shader_show_normal_scene>(this));
}

void geometry_shader_scene::render() {
  // Render current sub-scene
  m_sub_scene_manager.render();
}

void geometry_shader_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Geometry Shader");
  ImGui::Spacing();

  render_camera_ui();

  // Sub-scene selection UI
  ImGui::Separator();
  auto names = m_sub_scene_manager.get_names();
  int current_index = m_sub_scene_manager.current_index();
  if (ImGui::Combo("Scene Mode", &current_index, names.data(),
                   static_cast<int>(names.size()))) {
    m_sub_scene_manager.set_current(current_index);
  }

  // Render current sub-scene UI
  m_sub_scene_manager.render_ui();
}
