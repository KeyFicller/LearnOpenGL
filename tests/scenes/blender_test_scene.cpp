#include "blender_test_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_quad.h"
#include "tests/component/shader_loader.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

blender_test_scene::blender_test_scene()
    : renderable_scene_base("Blender Test Scene") {}

void blender_test_scene::init(GLFWwindow *window) {
  renderable_scene_base::init(window);

  // Setup mesh using quad helper
  prefab_quad::quad_mesh_data quad_data(
      prefab_quad::vertex_format::position_normal);
  setup_mesh(quad_data.mesh);

  // Load shaders using helper
  load_shader_pair("shaders/blender_test_test/vertex.shader",
                   "shaders/blender_test_test/fragment.shader",
                   "shaders/blender_test_test/light_fragment.shader", m_shader,
                   m_light_shader);

  // Initialize camera
  m_camera.m_position = {0.0f, 0.0f, 3.0f};
  m_camera.m_yaw = -90.0f; // Default looking along -Z axis
  m_camera.m_pitch = 0.0f;
  m_camera.update_view_matrix();
}

void blender_test_scene::render() {
  if (!m_shader) {
    return;
  }

  if (m_enable_blend) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  // Set matrices and render object
  // To make blend works, you should draw the far objects at first, then draw
  // the near ojects.

  // 1. Draw all the non-transparent objects.
  // 2. Sort the transparent object from far to near.
  // 3. Draw them with sorted order.
  set_matrices(m_shader,
               glm::translate(glm::mat4(1.0f), m_back_object_position));
  m_shader->set_uniform("uTransparency", m_transparency);
  m_shader->set_uniform("uObjectColor", m_back_object_color);
  m_mesh.draw();

  set_matrices(m_shader,
               glm::translate(glm::mat4(1.0f), m_front_object_position));
  m_shader->set_uniform("uTransparency", m_transparency);
  m_shader->set_uniform("uObjectColor", m_front_object_color);
  m_mesh.draw();

  if (!m_enable_blend)
    glDisable(GL_BLEND);
}

void blender_test_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Blender Test Scene");
  ImGui::Spacing();

  render_camera_ui();
  ImGui::Separator();
  ImGui::Text("Object Position");
  ImGui::SliderFloat2("Front Object Position", &m_front_object_position.x,
                      -3.0f, 3.0f);
  ImGui::SliderFloat2("Back Object Position", &m_back_object_position.x, -3.0f,
                      3.0f);
  ImGui::SliderFloat3("Front Object Color", &m_front_object_color.x, 0.0f,
                      1.0f);
  ImGui::SliderFloat3("Back Object Color", &m_back_object_color.x, 0.0f, 1.0f);
  ImGui::SliderFloat("Transparency", &m_transparency, 0.0f, 1.0f);
  ImGui::Checkbox("Blend", &m_enable_blend);
}