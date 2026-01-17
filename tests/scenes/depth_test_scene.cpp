#include "depth_test_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "mesh_helper.h"
#include "shader_helper.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

// Cube vertices: 24 vertices (6 faces * 4 vertices), each with 6 floats
// (position: 3, normal: 3)
static float box_vertices[] = {
    // Back face (z = -0.5), normal: (0, 0, -1)
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, // 0: back bottom left
    0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f,  // 1: back bottom right
    0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,   // 2: back top right
    -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f,  // 3: back top left

    // Front face (z = 0.5), normal: (0, 0, 1)
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,  // 4: front bottom right
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, // 5: front bottom left
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,  // 6: front top left
    0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,   // 7: front top right

    // Left face (x = -0.5), normal: (-1, 0, 0)
    -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f,  // 8: left bottom front
    -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, // 9: left bottom back
    -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f,  // 10: left top back
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,   // 11: left top front

    // Right face (x = 0.5), normal: (1, 0, 0)
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, // 12: right bottom back
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,  // 13: right bottom front
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,   // 14: right top front
    0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,  // 15: right top back

    // Bottom face (y = -0.5), normal: (0, -1, 0)
    -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,  // 16: bottom front left
    0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f,   // 17: bottom front right
    0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f,  // 18: bottom back right
    -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, // 19: bottom back left

    // Top face (y = 0.5), normal: (0, 1, 0)
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, // 20: top back left
    0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  // 21: top back right
    0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,   // 22: top front right
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,  // 23: top front left
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

static const char *depth_test_mode_names[] = {
    "ALWAYS", "NEVER",   "LESS",     "EQUAL",
    "LEQUAL", "GREATER", "NOTEQUAL", "GEQUAL",
};

static std::vector<int> depth_test_modes = {
    GL_ALWAYS, GL_NEVER,   GL_LESS,     GL_EQUAL,
    GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL,
};

depth_test_scene::depth_test_scene()
    : renderable_scene_base("Depth Test Scene") {}

void depth_test_scene::init(GLFWwindow *_window) {
  renderable_scene_base::init(_window);

  // Setup mesh using helper
  mesh_data data;
  data.vertices = box_vertices;
  data.vertex_size = sizeof(box_vertices);
  data.indices = box_indices;
  data.index_count = sizeof(box_indices) / sizeof(unsigned int);
  data.attributes = {{3, GL_FLOAT, GL_FALSE}, {3, GL_FLOAT, GL_FALSE}};
  setup_mesh(data);

  // Load shaders using helper
  load_shader_pair("shaders/depth_test_test/vertex.shader",
                   "shaders/depth_test_test/fragment.shader",
                   "shaders/depth_test_test/light_fragment.shader", m_shader,
                   m_light_shader);

  // Initialize camera - position at (10, 10, 10) looking at origin
  m_camera.m_position = {10.0f, 10.0f, 10.0f};
  m_camera.m_yaw = -135.0f; // Looking from (10,10,10) towards (0,0,0)
  m_camera.m_pitch = -35.26f;
  m_camera.update_view_matrix();
}

void depth_test_scene::render() {
  if (!m_shader) {
    return;
  }

  glDepthFunc(depth_test_modes[m_depth_test_index]);
  // Set matrices and render object
  m_shader->use();
  m_shader->set_uniform<float, 3>("uLightColor", &m_light_color.x);
  m_shader->set_uniform<float, 3>("uObjectColor", &m_object_color.x);
  m_shader->set_uniform<float, 1>("uAmbientStrength", &m_ambient_strength);
  m_shader->set_uniform<float, 3>("uLightPosition", &m_light_position.x);
  m_shader->set_uniform<float, 1>("uSpecularStrength", &m_specular_strength);
  m_shader->set_uniform<float, 3>("uEyePosition", &m_camera.m_position.x);
  m_shader->set_uniform<float, 1>("uShininess", &m_shininess);
  m_shader->set_uniform<bool, 1>("uDepthVisualization", &m_depth_visualization);
  m_shader->set_uniform<float, 1>("uNear", &m_camera.m_near);
  m_shader->set_uniform<float, 1>("uFar", &m_camera.m_far);
  {
    set_matrices(m_shader, glm::translate(glm::mat4(1.0f), m_object_position));
    m_mesh.draw();
  }

  {
    set_matrices(m_shader, glm::translate(glm::mat4(1.0f), m_object_position2));
    m_mesh.draw();
  }
  glDepthFunc(GL_LESS);

  // Render light source using helper
  render_light_source(m_light_shader, m_light_position);
}

void depth_test_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Depth Test Scene");
  ImGui::Spacing();

  render_camera_ui();
  ImGui::SliderFloat3("Object Position", &m_object_position.x, -1.0f, 1.0f);
  ImGui::SliderFloat3("Object Position 2", &m_object_position2.x, -1.0f, 1.0f);

  ImGui::Combo("Depth Test Mode", &m_depth_test_index, depth_test_mode_names,
               IM_ARRAYSIZE(depth_test_mode_names));

  ImGui::Checkbox("Depth Visualization", &m_depth_visualization);
}
