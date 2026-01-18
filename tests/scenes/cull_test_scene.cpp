#include "cull_test_scene.h"

#include "glad/gl.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_cube.h"
#include "tests/component/prefab_quad.h"
#include "tests/component/shader_loader.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

cull_test_scene::cull_test_scene() : renderable_scene_base("Cull Test Scene") {}

void cull_test_scene::init(GLFWwindow *window) {
  renderable_scene_base::init(window);

  // Setup mesh using quad helper
  prefab_cube::cube_mesh_data cube_data(
      prefab_cube::vertex_format::position_normal);
  setup_mesh(cube_data.mesh);

  // Load shaders using helper
  load_shader_pair("shaders/cull_test_test/vertex.shader",
                   "shaders/cull_test_test/fragment.shader",
                   "shaders/cull_test_test/light_fragment.shader", m_shader,
                   m_light_shader);

  // Initialize camera
  m_camera.m_position = {0.0f, 0.0f, 3.0f};
  m_camera.m_yaw = -90.0f; // Default looking along -Z axis
  m_camera.m_pitch = 0.0f;
  m_camera.update_view_matrix();
}

void cull_test_scene::render() {
  if (!m_shader) {
    return;
  }

  if (m_enable_culling) {
    glEnable(GL_CULL_FACE);
    if (m_cull_back_faces)
      glCullFace(GL_BACK);
    else
      glCullFace(GL_FRONT);
    glFrontFace(GL_CCW);
  }

  set_matrices(m_shader);
  m_shader->set_uniform("uObjectColor", glm::vec3(1.0f, 1.0f, 1.0f));
  m_mesh.draw();
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  set_matrices(m_shader, glm::scale(glm::mat4(1.0f), glm::vec3(1.01f)));
  m_shader->set_uniform("uObjectColor", glm::vec3(0.0f, 0.0f, 0.0f));
  m_mesh.draw();
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  if (!m_enable_culling)
    glDisable(GL_CULL_FACE);
}

void cull_test_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Cull Test Scene");
  ImGui::Spacing();

  render_camera_ui();
  ImGui::Separator();
  ImGui::Checkbox("Enable Culling", &m_enable_culling);
  ImGui::Checkbox("Cull Back Faces", &m_cull_back_faces);
}