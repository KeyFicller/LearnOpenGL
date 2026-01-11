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
    : test_scene_base("Transform Test"), m_transform(glm::mat4(1.0f)), m_VAO(0),
      m_VBO(0), m_shader(nullptr), m_rotation_speed(1.0f),
      m_rotation_axis(glm::vec3(0.0f, 0.0f, 1.0f)) {}

transform_test_scene::~transform_test_scene() {
  if (m_VAO) {
    glDeleteVertexArrays(1, &m_VAO);
  }
  if (m_VBO) {
    glDeleteBuffers(1, &m_VBO);
  }
  delete m_shader;
}

void transform_test_scene::init() {
  // Create and bind VAO
  glGenVertexArrays(1, &m_VAO);
  glBindVertexArray(m_VAO);

  // Create and bind VBO
  glGenBuffers(1, &m_VBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices,
               GL_STATIC_DRAW);

  // Set vertex attributes (position + color, no texture)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

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
  m_shader->set_uniform<glm::mat4>("transform", m_transform);
  glBindVertexArray(m_VAO);
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
