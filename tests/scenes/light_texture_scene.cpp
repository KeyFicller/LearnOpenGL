#include "light_texture_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "mesh_helper.h"
#include "shader_helper.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

// Cube vertices: 24 vertices (6 faces * 4 vertices), each with 8 floats
// (position: 3, normal: 3, texcoord: 2)
static float box_vertices[] = {
    // Back face (z = -0.5), normal: (0, 0, -1)
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // 0: back bottom left
    0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,  // 1: back bottom right
    0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,   // 2: back top right
    -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,  // 3: back top left

    // Front face (z = 0.5), normal: (0, 0, 1)
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // 4: front bottom right
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, // 5: front bottom left
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,  // 6: front top left
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,   // 7: front top right

    // Left face (x = -0.5), normal: (-1, 0, 0)
    -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // 8: left bottom front
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // 9: left bottom back
    -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // 10: left top back
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,   // 11: left top front

    // Right face (x = 0.5), normal: (1, 0, 0)
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // 12: right bottom back
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // 13: right bottom front
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // 14: right top front
    0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // 15: right top back

    // Bottom face (y = -0.5), normal: (0, -1, 0)
    -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // 16: bottom front left
    0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,  // 17: bottom front right
    0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // 18: bottom back right
    -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // 19: bottom back left

    // Top face (y = 0.5), normal: (0, 1, 0)
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // 20: top back left
    0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // 21: top back right
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,   // 22: top front right
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,  // 23: top front left
};

static unsigned int box_indices[] = {
    // Back face
    0,
    1,
    2,
    2,
    3,
    0,
    // Front face
    4,
    5,
    6,
    6,
    7,
    4,
    // Left face
    8,
    9,
    10,
    10,
    11,
    8,
    // Right face
    12,
    13,
    14,
    14,
    15,
    12,
    // Bottom face
    16,
    17,
    18,
    18,
    19,
    16,
    // Top face
    20,
    21,
    22,
    22,
    23,
    20,
};

light_texture_scene::light_texture_scene()
    : renderable_scene_base("Light Texture Test") {}

void light_texture_scene::init(GLFWwindow *_window) {
  renderable_scene_base::init(_window);

  // Setup mesh using helper
  mesh_data data;
  data.vertices = box_vertices;
  data.vertex_size = sizeof(box_vertices);
  data.indices = box_indices;
  data.index_count = sizeof(box_indices) / sizeof(unsigned int);
  data.attributes = {{3, GL_FLOAT, GL_FALSE},
                     {3, GL_FLOAT, GL_FALSE},
                     {2, GL_FLOAT, GL_FALSE}};
  setup_mesh(data);

  // Load shaders using helper
  load_shader_pair("shaders/light_texture_test/vertex.shader",
                   "shaders/light_texture_test/fragment.shader",
                   "shaders/light_texture_test/light_fragment.shader",
                   m_shader, m_light_shader);

  // Initialize camera
  m_camera.m_position = {3.0f, 3.0f, 0.0f};
  m_camera.m_front = glm::vec3{0.0f, 0.0f, 0.0f} - m_camera.m_position;
  m_camera.m_up = glm::normalize(glm::vec3{0.0f, 0.0f, 1.0f});
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
