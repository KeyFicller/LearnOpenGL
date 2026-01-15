#include "camera_test_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "mesh_helper.h"
#include "shader_helper.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

// Cube vertices: 8 vertices, each with 3 coordinates (x, y, z)
static float box_vertices[] = {
    // Back face (z = -0.5)
    -0.5f, -0.5f, -0.5f, // 0: back bottom left
    0.5f, -0.5f, -0.5f,  // 1: back bottom right
    0.5f, 0.5f, -0.5f,   // 2: back top right
    -0.5f, 0.5f, -0.5f,  // 3: back top left
    // Front face (z = 0.5)
    -0.5f, -0.5f, 0.5f, // 4: front bottom left
    0.5f, -0.5f, 0.5f,  // 5: front bottom right
    0.5f, 0.5f, 0.5f,   // 6: front top right
    -0.5f, 0.5f, 0.5f,  // 7: front top left
};

static unsigned int box_indices[] = {
    // Back face
    0, 1, 2, 2, 3, 0,
    // Front face
    4, 5, 6, 6, 7, 4,
    // Left face
    7, 4, 0, 0, 3, 7,
    // Right face
    1, 5, 6, 6, 2, 1,
    // Bottom face
    4, 5, 1, 1, 0, 4,
    // Top face
    3, 2, 6, 6, 7, 3};

camera_test_scene::camera_test_scene()
    : camera_scene_base("Camera Test") {}

void camera_test_scene::init(GLFWwindow *_window) {
  camera_scene_base::init(_window);

  // Setup mesh using helper
  mesh_data data;
  data.vertices = box_vertices;
  data.vertex_size = sizeof(box_vertices);
  data.indices = box_indices;
  data.index_count = sizeof(box_indices) / sizeof(unsigned int);
  data.attributes = {{3, GL_FLOAT, GL_FALSE}};
  m_mesh.setup_mesh(data);

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
  m_shader->set_uniform<glm::mat4, 1>("model", &model);
  m_shader->set_uniform<glm::mat4, 1>("view", &m_camera.m_view_matrix);
  m_shader->set_uniform<glm::mat4, 1>("projection",
                                      &m_camera.m_projection_matrix);

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
      ImGui::SliderFloat3("eye", &m_camera.m_position[0], -10.0f, 10.0f);
  update_view_matrix |=
      ImGui::SliderFloat3("front", &m_camera.m_front[0], -1.0f, 1.0f);
  update_view_matrix |=
      ImGui::SliderFloat3("up", &m_camera.m_up[0], -1.0f, 1.0f);
  ImGui::SameLine();
  if (ImGui::Button("Normalize Up")) {
    m_camera.m_up = glm::normalize(m_camera.m_up);
    update_view_matrix = true;
  }
  if (update_view_matrix) {
    m_camera.update_view_matrix();
  }

  // Projection matrix controls
  bool update_projection_matrix = false;
  update_projection_matrix |=
      ImGui::SliderFloat("fov", &m_camera.m_fov, 0.0f, 180.0f);
  update_projection_matrix |=
      ImGui::SliderFloat("aspect ratio", &m_camera.m_aspect_ratio, 0.1f, 10.0f);
  update_projection_matrix |=
      ImGui::SliderFloat("near", &m_camera.m_near, 0.01f, 10.0f);
  update_projection_matrix |=
      ImGui::SliderFloat("far", &m_camera.m_far, 10.0f, 100.0f);
  if (update_projection_matrix) {
    m_camera.update_projection_matrix();
  }
}
