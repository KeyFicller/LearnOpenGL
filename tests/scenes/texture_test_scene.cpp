#include "texture_test_scene.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>

// Vertices for quad (texture test)
static float texture_quad_vertices[] = {
    0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right
    0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom right
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
    -0.5f, 0.5f,  0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f  // top left
};

static unsigned int texture_quad_indices[] = {
    0, 1, 3, // first triangle
    1, 2, 3  // second triangle
};

texture_test_scene::texture_test_scene()
    : test_scene_base("Texture Test"), m_VAO(0), m_VBO(0), m_EBO(0),
      m_shader(nullptr), m_texture1(nullptr), m_texture2(nullptr),
      m_mix_ratio(0.2f) {}

texture_test_scene::~texture_test_scene() {
  if (m_VAO) {
    glDeleteVertexArrays(1, &m_VAO);
  }
  if (m_VBO) {
    glDeleteBuffers(1, &m_VBO);
  }
  if (m_EBO) {
    glDeleteBuffers(1, &m_EBO);
  }
  delete m_shader;
  delete m_texture1;
  delete m_texture2;
}

void texture_test_scene::init(GLFWwindow *_window) {
  test_scene_base::init(_window);
  // Create and bind VAO
  glGenVertexArrays(1, &m_VAO);
  glBindVertexArray(m_VAO);

  // Create and bind VBO
  glGenBuffers(1, &m_VBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(texture_quad_vertices),
               texture_quad_vertices, GL_STATIC_DRAW);

  // Create and bind EBO
  glGenBuffers(1, &m_EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(texture_quad_indices),
               texture_quad_indices, GL_STATIC_DRAW);

  // Set vertex attributes
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                        (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  // Load shader
  try {
    m_shader = new shader("shaders/texture_test/vertex.shader",
                          "shaders/texture_test/fragment.shader");
  } catch (const std::exception &e) {
    std::cerr << "Failed to load texture test shader: " << e.what()
              << std::endl;
    m_shader = nullptr;
  }

  // Load textures
  try {
    m_texture1 = new texture_2d("assets/images/spider_man.jpeg");
    m_texture1->set_wrap_mode(texture_2d::wrap_mode::k_repeat);
    m_texture1->set_filter_mode(texture_2d::filter_mode::k_nearest);

    m_texture2 = new texture_2d("assets/images/sea.jpeg");
    m_texture2->set_wrap_mode(texture_2d::wrap_mode::k_repeat);
    m_texture2->set_filter_mode(texture_2d::filter_mode::k_nearest);
  } catch (const std::exception &e) {
    std::cerr << "Failed to load texture: " << e.what() << std::endl;
  }
}

void texture_test_scene::render() {
  if (!m_shader || !m_texture1 || !m_texture2) {
    return;
  }

  m_shader->use();
  float time = glfwGetTime();
  m_shader->set_uniform<float>("ourColor", sin(time), 0.0f, 0.0f, 1.0f);
  m_texture1->bind(0);
  m_texture2->bind(1);
  m_shader->set_uniform<int>("texture1", 0);
  m_shader->set_uniform<int>("texture2", 1);
  glBindVertexArray(m_VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void texture_test_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Texture Controls");
  ImGui::Spacing();
  ImGui::SliderFloat("Mix Ratio", &m_mix_ratio, 0.0f, 1.0f);
}
