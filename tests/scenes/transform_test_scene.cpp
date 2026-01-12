#include "transform_test_scene.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// Vertices for triangle (transform test)
static float triangle_vertices[] = {
    0.0f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, // top
    0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom right
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f  // bottom left
};

transform_test_scene::transform_test_scene()
    : test_scene_base("Transform Test"), m_transform(glm::mat4(1.0f)),
      m_shader(nullptr), m_rotation_speed(1.0f),
      m_rotation_axis(glm::vec3(0.0f, 0.0f, 1.0f)) {}

transform_test_scene::~transform_test_scene() {
  delete m_VAO;
  delete m_VBO;
  delete m_shader;
}

void transform_test_scene::init(GLFWwindow *_window) {
  test_scene_base::init(_window);

  // Create VAO
  m_VAO = new vertex_array_object();
  m_VAO->bind();

  // Create and bind VBO
  m_VBO = new vertex_buffer_object();
  m_VBO->bind();
  m_VBO->set_data(triangle_vertices, sizeof(triangle_vertices), GL_STATIC_DRAW);

  // Set vertex attributes (position + color: 3 floats position, 3 floats color)
  m_VAO->add_attributes({{3, GL_FLOAT, GL_FALSE}, {3, GL_FLOAT, GL_FALSE}});

  // Load shader
  try {
    m_shader = new shader("shaders/transform_test/vertex.shader",
                          "shaders/transform_test/fragment.shader");
  } catch (const std::exception &e) {
    std::cerr << "Failed to load transform test shader: " << e.what()
              << std::endl;
    m_shader = nullptr;
  }
}

void transform_test_scene::render() {
  if (!m_shader) {
    return;
  }

  float time = glfwGetTime();
  float angle = time * glm::radians(m_rotation_speed);
  m_transform = glm::rotate(glm::mat4(1.0f), angle, m_rotation_axis);

  m_shader->use();
  m_shader->set_uniform<glm::mat4, 1>("transform", &m_transform);
  m_VAO->bind();
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void transform_test_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Transform Controls");
  ImGui::Spacing();

  ImGui::SliderFloat("Rotation Speed", &m_rotation_speed, 0.0f, 10.0f);
  ImGui::SliderFloat3("Rotation Axis", &m_rotation_axis[0], -1.0f, 1.0f);

  ImGui::Spacing();
}
