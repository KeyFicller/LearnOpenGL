#include "color_test_scene.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>

// Vertices for quad (color test)
static float color_quad_vertices[] = {
    0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, // top right
    0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom right
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom left
    -0.5f, 0.5f,  0.0f, 1.0f, 1.0f, 1.0f  // top left
};

static unsigned int color_quad_indices[] = {
    0, 1, 3, // first triangle
    1, 2, 3  // second triangle
};

color_test_scene::color_test_scene()
    : test_scene_base("Color Test"), m_shader(nullptr), m_animate(true) {
  m_color[0] = 1.0f; // r
  m_color[1] = 0.0f; // g
  m_color[2] = 0.0f; // b
}

color_test_scene::~color_test_scene() {
  delete m_VAO;
  delete m_VBO;
  delete m_EBO;
  delete m_shader;
}

void color_test_scene::init(GLFWwindow *_window) {
  test_scene_base::init(_window);

  // Create VAO
  m_VAO = new vertex_array();
  m_VAO->bind();

  // Create and bind VBO
  m_VBO = new vertex_buffer();
  m_VBO->bind();
  m_VBO->set_data(color_quad_vertices, sizeof(color_quad_vertices),
                  GL_STATIC_DRAW);

  // Create and bind EBO
  m_EBO = new index_buffer();
  m_EBO->bind();
  m_EBO->set_data(color_quad_indices, sizeof(color_quad_indices));

  // Set vertex attributes (position + color: 3 floats position, 3 floats color)
  m_VAO->add_attributes({{3, GL_FLOAT, GL_FALSE}, {3, GL_FLOAT, GL_FALSE}});

  // Load shader
  try {
    m_shader = new shader("shaders/color_test/vertex.shader",
                          "shaders/color_test/fragment.shader");
  } catch (const std::exception &e) {
    std::cerr << "Failed to load color test shader: " << e.what() << std::endl;
    m_shader = nullptr;
  }
}

void color_test_scene::render() {
  if (!m_shader) {
    return;
  }

  m_shader->use();
  float r, g, b;
  if (m_animate) {
    float time = glfwGetTime();
    r = (sin(time) + 1.0f) * 0.5f;
    g = (sin(time + 2.0f) + 1.0f) * 0.5f;
    b = (sin(time + 4.0f) + 1.0f) * 0.5f;
  } else {
    r = m_color[0];
    g = m_color[1];
    b = m_color[2];
  }
  m_shader->set_uniform<float>("ourColor", r, g, b, 1.0f);
  m_VAO->bind();
  m_EBO->bind();
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void color_test_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Color Controls");
  ImGui::Spacing();

  ImGui::Checkbox("Animate Color", &m_animate);

  if (!m_animate) {
    ImGui::ColorEdit3("Color", m_color);
  } else {
    ImGui::Text("Color is animating");
  }
}
