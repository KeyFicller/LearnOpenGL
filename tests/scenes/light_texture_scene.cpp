#include "light_texture_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "tests/component/prefab_cube.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/shader_loader.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

light_texture_scene::light_texture_scene()
    : renderable_scene_base("Light Texture Test") {}

void light_texture_scene::init(GLFWwindow *_window) {
  renderable_scene_base::init(_window);

  // Setup mesh using cube helper
  prefab_cube::cube_mesh_data cube_data(
      prefab_cube::vertex_format::position_normal_tex);
  setup_mesh(cube_data.mesh);

  // Load shaders using helper
  load_shader_pair("shaders/light_texture_test/vertex.shader",
                   "shaders/light_texture_test/fragment.shader",
                   "shaders/light_texture_test/light_fragment.shader",
                   m_shader, m_light_shader);

  // Initialize camera - position at (3, 3, 0) looking at origin
  m_camera.m_position = {3.0f, 3.0f, 0.0f};
  m_camera.m_yaw = 180.0f;    // Looking from (3,3,0) towards (0,0,0)
  m_camera.m_pitch = -45.0f;
  m_camera.update_view_matrix();

  // Load textures
  m_texture_material.m_diffuse_texture =
      new texture_2d("assets/images/spider_man.jpeg");
  m_texture_material.m_specular_texture =
      new texture_2d("assets/images/spider_man.jpeg");
}

void light_texture_scene::render() {
  if (!m_shader) {
    return;
  }

  // Set matrices and render object
  set_matrices(m_shader);
  uniform(*m_shader, m_light, "uLight");
  uniform(*m_shader, m_texture_material, "uMaterial");
  m_shader->set_uniform<glm::vec3, 1>("uEyePosition", &m_camera.m_position);
  m_mesh.draw();

  // Render light source using helper
  render_light_source(m_light_shader, m_light.m_position);
}

void light_texture_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Light Texture Test");
  ImGui::Spacing();

  render_camera_ui();
  ImGui::PushID("Light");
  ImGui::Text("Light");
  ui(m_light);
  ImGui::PopID();
  ImGui::PushID("Material");
  ImGui::Text("Material");
  ui(m_texture_material);
  ImGui::PopID();

  ImGui::Separator();
}
