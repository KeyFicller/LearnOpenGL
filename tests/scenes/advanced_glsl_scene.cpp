#include "advanced_glsl_scene.h"

#include "advanced_glsl_sub_scenes.h"
#include "glad/gl.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_cube.h"
#include "tests/component/prefab_quad.h"
#include "tests/component/shader_loader.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

advanced_glsl_scene::advanced_glsl_scene()
    : renderable_scene_base("Advanced GLSL") {}

void advanced_glsl_scene::init(GLFWwindow *_window) {
  renderable_scene_base::init(_window);

  // Setup mesh using cube helper
  prefab_cube::cube_mesh_data cube_data(
      prefab_cube::vertex_format::position_normal);
  setup_mesh(*cube_data.mesh);

  // Load shaders using helper
  load_shader_pair("shaders/advanced_glsl_test/vertex.shader",
                   "shaders/advanced_glsl_test/fragment.shader",
                   "shaders/advanced_glsl_test/light_fragment.shader", m_shader,
                   m_light_shader);

  // Initialize camera - position at (10, 10, 10) looking at origin
  m_camera.m_position = {10.0f, 10.0f, 10.0f};
  m_camera.m_yaw = -135.0f; // Looking from (10,10,10) towards (0,0,0)
  m_camera.m_pitch = -35.26f;
  m_camera.update_view_matrix();

  // Initialize sub-scenes
  m_sub_scene_manager.add_sub_scene(
      std::make_unique<advanced_glsl_vertex_variable_sub_scene>(this));
  m_sub_scene_manager.add_sub_scene(
      std::make_unique<advanced_glsl_fragment_variable_sub_scene>(this));
  m_sub_scene_manager.add_sub_scene(
      std::make_unique<advanced_glsl_uniform_buffer_sub_scene>(this));
}

void advanced_glsl_scene::render() {
  if (!m_shader) {
    return;
  }

  // Render current sub-scene
  m_sub_scene_manager.render();
}

void advanced_glsl_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Advanced GLSL");
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
