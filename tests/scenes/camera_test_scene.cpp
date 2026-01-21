#include "camera_test_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_cube.h"
#include "tests/component/shader_loader.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

camera_test_scene::camera_test_scene() : camera_scene_base("Camera Test") {}

void camera_test_scene::init(GLFWwindow *_window) {
  camera_scene_base::init(_window);

  // Setup mesh using cube helper
  prefab_cube::cube_mesh_data cube_data(
      prefab_cube::vertex_format::position_only);
  m_mesh.setup_mesh(*cube_data.mesh);

  // Load shader using helper
  m_shader = load_shader("shaders/camera_test/vertex.shader",
                         "shaders/camera_test/fragment.shader");
}

void camera_test_scene::render() {
  if (!m_shader) {
    return;
  }

  m_shader->use();
  glm::mat4 model = glm::mat4(1.0f);
  m_shader->set_uniform("model", model);
  m_shader->set_uniform("view", m_camera.ViewMatrix);
  m_shader->set_uniform("projection", m_camera.ProjectionMatrix);

  m_mesh.draw();
}

void camera_test_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Camera Test");
  ImGui::Spacing();

  render_camera_ui();

  // View matrix controls
  bool update_view_matrix = false;
  update_view_matrix |=
      ImGui::SliderFloat3("Position", &m_camera.Position[0], -10.0f, 10.0f);
  update_view_matrix |=
      ImGui::SliderFloat("Yaw", &m_camera.Yaw, -180.0f, 180.0f);
  update_view_matrix |=
      ImGui::SliderFloat("Pitch", &m_camera.Pitch, -89.0f, 89.0f);
  if (update_view_matrix) {
    m_camera.update_view_matrix();
  }

  // Projection matrix controls
  bool update_projection_matrix = false;
  update_projection_matrix |=
      ImGui::SliderFloat("fov", &m_camera.FOV, 0.0f, 180.0f);
  update_projection_matrix |=
      ImGui::SliderFloat("aspect ratio", &m_camera.AspectRatio, 0.1f, 10.0f);
  update_projection_matrix |=
      ImGui::SliderFloat("near", &m_camera.Near, 0.01f, 10.0f);
  update_projection_matrix |=
      ImGui::SliderFloat("far", &m_camera.Far, 10.0f, 100.0f);
  if (update_projection_matrix) {
    m_camera.update_projection_matrix();
  }
}
