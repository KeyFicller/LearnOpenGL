#include "coordinate_test_scene.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// Vertices for quad (color test)
static float color_quad_vertices[] = {0.5f,  0.5f,  0.0f, 0.5f,  -0.5f, 0.0f,
                                      -0.5f, -0.5f, 0.0f, -0.5f, 0.5f,  0.0f};

static unsigned int color_quad_indices[] = {
    0, 1, 3, // first triangle
    1, 2, 3  // second triangle
};

coordinate_test_scene::coordinate_test_scene()
    : test_scene_base("Coordinate Test"), m_shader(nullptr) {}

coordinate_test_scene::~coordinate_test_scene() {
  delete m_VAO;
  delete m_VBO;
  delete m_EBO;
  delete m_shader;
}

void coordinate_test_scene::init(GLFWwindow *_window) {
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

  // Set vertex attributes (position only, 3 floats)
  m_VAO->add_attributes({{3, GL_FLOAT, GL_FALSE}});

  // Load shader
  try {
    m_shader = new shader("shaders/coordinate_test/vertex.shader",
                          "shaders/coordinate_test/fragment.shader");
  } catch (const std::exception &e) {
    std::cerr << "Failed to load coordinate test shader: " << e.what()
              << std::endl;
    m_shader = nullptr;
  }
}

void coordinate_test_scene::render() {
  if (m_shader) {
    m_shader->use();
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::lookAt(m_camera_position, glm::vec3(0.0f, 0.0f, 0.0f),
                       glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 projection = glm::mat4(1.0f);
    projection =
        glm::perspective(glm::radians(m_fov), m_width / m_height, 0.1f, 100.0f);

    m_shader->set_uniform<glm::mat4, 1>("view", &view);
    m_shader->set_uniform<glm::mat4, 1>("projection", &projection);

    m_VAO->bind();
    m_EBO->bind();
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    for (const auto &position : m_positions) {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, position);
      model =
          glm::rotate(model, glm::radians(m_rotation_angle), m_rotation_axis);
      m_shader->set_uniform<glm::mat4, 1>("model", &model);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }
}

void coordinate_test_scene::render_ui() {
  ImGui::SliderFloat("Rotation Angle", &m_rotation_angle, 0.0f, 360.0f);
  ImGui::SliderFloat3("Rotation Axis", &m_rotation_axis[0], 0.0f, 1.0f);
  ImGui::SliderFloat3("Camera Position", &m_camera_position[0], -10.0f, 10.0f);
  ImGui::SliderFloat("FOV", &m_fov, 0.0f, 180.0f);
  ImGui::SliderFloat("Height", &m_height, 0.0f, 1000.0f);
  ImGui::SliderFloat("Width", &m_width, 0.0f, 1000.0f);
  ImGui::SliderInt("Editing Position Index", &m_editing_position_index, 0,
                   m_positions.size() - 1);
  ImGui::SliderFloat3("Editing Position",
                      &m_positions[m_editing_position_index][0], -10.0f, 10.0f);
}