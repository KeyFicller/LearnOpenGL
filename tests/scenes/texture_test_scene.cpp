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
    : test_scene_base("Texture Test"), m_shader(nullptr), m_texture1(nullptr),
      m_texture2(nullptr), m_mix_ratio(0.2f) {}

texture_test_scene::~texture_test_scene() {
  delete m_VAO;
  delete m_VBO;
  delete m_EBO;
  delete m_shader;
  delete m_texture1;
  delete m_texture2;
}

void texture_test_scene::init(GLFWwindow *_window) {
  test_scene_base::init(_window);

  // Create VAO
  m_VAO = new vertex_array_object();
  m_VAO->bind();

  // Create and bind VBO
  m_VBO = new vertex_buffer_object();
  m_VBO->bind();
  m_VBO->set_data(texture_quad_vertices, sizeof(texture_quad_vertices),
                  GL_STATIC_DRAW);

  // Create and bind EBO
  m_EBO = new index_buffer_object();
  m_EBO->bind();
  m_EBO->set_data(texture_quad_indices, sizeof(texture_quad_indices));

  // Set vertex attributes (position: 3 floats, color: 3 floats, texcoord: 2
  // floats)
  m_VAO->add_attributes({{3, GL_FLOAT, GL_FALSE},
                         {3, GL_FLOAT, GL_FALSE},
                         {2, GL_FLOAT, GL_FALSE}});

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
  m_VAO->bind();
  m_EBO->bind();
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void texture_test_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Texture Controls");
  ImGui::Spacing();
  ImGui::SliderFloat("Mix Ratio", &m_mix_ratio, 0.0f, 1.0f);
}
