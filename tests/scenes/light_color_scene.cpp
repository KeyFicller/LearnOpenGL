#include "light_color_scene.h"

#include <iostream>

#include "glad/gl.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/geometric.hpp"
#include "imgui.h"
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

light_color_scene::light_color_scene()
    : test_scene_base("Light Color Test"), m_shader(nullptr) {}

light_color_scene::~light_color_scene() {
  delete m_VAO;
  delete m_VBO;
  delete m_EBO;
  delete m_shader;
}

void light_color_scene::init(GLFWwindow *_window) {
  test_scene_base::init(_window);

  // Create VAO
  m_VAO = new vertex_array_object();
  m_VAO->bind();

  // Create and bind VBO
  m_VBO = new vertex_buffer_object();
  m_VBO->bind();
  m_VBO->set_data(box_vertices, sizeof(box_vertices), GL_STATIC_DRAW);

  // Create and bind EBO
  m_EBO = new index_buffer_object();
  m_EBO->bind();
  m_EBO->set_data(box_indices, sizeof(box_indices));

  // Set vertex attributes (position: 3 floats, normal: 3 floats)
  m_VAO->add_attributes({{3, GL_FLOAT, GL_FALSE}, {3, GL_FLOAT, GL_FALSE}});

  // Load shader
  try {
    m_shader = new shader("shaders/light_color_test/vertex.shader",
                          "shaders/light_color_test/fragment.shader");
    m_light_shader =
        new shader("shaders/light_color_test/vertex.shader",
                   "shaders/light_color_test/light_fragment.shader");
  } catch (const std::exception &e) {
    std::cerr << "Failed to load camera test shader: " << e.what() << std::endl;
    m_shader = nullptr;
    m_light_shader = nullptr;
  }

  m_camera_controller = new camera_controller(m_camera, _window);

  m_camera.m_position = {10.0f, 10.0f, 10.0f};
  m_camera.m_front = glm::vec3{0.0f, 0.0f, 0.0f} - m_camera.m_position;
  m_camera.m_up = glm::normalize(glm::vec3{-1, -1, 2});
  m_camera.update_view_matrix();
}

void light_color_scene::render() {
  if (!m_shader) {
    return;
  }

  m_shader->use();
  glm::mat4 model = glm::mat4(1.0f);
  m_shader->set_uniform<glm::mat4, 1>("model", &model);
  m_shader->set_uniform<glm::mat4, 1>("view", &m_camera.m_view_matrix);
  m_shader->set_uniform<glm::mat4, 1>("projection",
                                      &m_camera.m_projection_matrix);

  m_shader->set_uniform<float, 3>("uLightColor", &m_light_color.x);
  m_shader->set_uniform<float, 3>("uObjectColor", &m_object_color.x);
  m_shader->set_uniform<float, 1>("uAmbientStrength", &m_ambient_strength);
  m_shader->set_uniform<float, 3>("uLightPosition", &m_light_position.x);
  m_shader->set_uniform<float, 1>("uSpecularStrength", &m_specular_strength);
  m_shader->set_uniform<float, 3>("uEyePosition", &m_camera.m_position.x);
  m_shader->set_uniform<float, 1>("uShininess", &m_shininess);
  m_VAO->bind();
  m_EBO->bind();
  glDrawElements(GL_TRIANGLES, sizeof(box_indices) / sizeof(unsigned int),
                 GL_UNSIGNED_INT, 0);

  // Render light source
  m_light_shader->use();
  glm::mat4 light_model = glm::mat4(1.0f);
  light_model = glm::translate(light_model, m_light_position);
  light_model = glm::scale(light_model, glm::vec3{0.2f, 0.2f, 0.2f});
  m_light_shader->set_uniform<glm::mat4, 1>("model", &light_model);
  m_light_shader->set_uniform<glm::mat4, 1>("view", &m_camera.m_view_matrix);
  m_light_shader->set_uniform<glm::mat4, 1>("projection",
                                            &m_camera.m_projection_matrix);
  m_VAO->bind();
  m_EBO->bind();
  glDrawElements(GL_TRIANGLES, sizeof(box_indices) / sizeof(unsigned int),
                 GL_UNSIGNED_INT, 0);
}

void light_color_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Light Color Test");
  ImGui::Spacing();

  ImGui::Checkbox("Camera Controller", &m_camera_controller_enabled);
  ImGui::SliderFloat3("Light Color", &m_light_color.x, 0.0f, 1.0f);
  ImGui::SliderFloat3("Object Color", &m_object_color.x, 0.0f, 1.0f);
  ImGui::SliderFloat3("Light Position", &m_light_position.x, -10.0f, 10.0f);

  ImGui::Separator();
  ImGui::SliderFloat("Ambient Strength", &m_ambient_strength, 0.0f, 1.0f,
                     "%.2f");
  ImGui::SliderFloat("Specular Strength", &m_specular_strength, 0.0f, 1.0f,
                     "%.2f");
  ImGui::SliderFloat("Shininess", &m_shininess, 1.0f, 128.0f, "%.0f");
}

bool light_color_scene::on_mouse_moved(double _xpos, double _ypos) {
  if (m_camera_controller_enabled)
    m_camera_controller->on_mouse_moved(_xpos, _ypos);
  return false;
}

bool light_color_scene::on_mouse_scroll(double _xoffset, double _yoffset) {
  if (m_camera_controller_enabled)
    m_camera_controller->on_mouse_scroll(_xoffset, _yoffset);
  return false;
}

void light_color_scene::update(float _delta_time) {
  if (m_camera_controller_enabled)
    m_camera_controller->update(_delta_time);
}