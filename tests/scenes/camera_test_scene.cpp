#include "camera_test_scene.h"

#include <iostream>

#include "imgui.h"
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
    : test_scene_base("Camera Test"), m_VAO(0), m_VBO(0), m_EBO(0),
      m_shader(nullptr) {}

camera_test_scene::~camera_test_scene() {
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
}

void camera_test_scene::init(GLFWwindow *_window) {
  test_scene_base::init(_window);
  // Create and bind VAO
  glGenVertexArrays(1, &m_VAO);
  glBindVertexArray(m_VAO);

  // Create and bind VBO
  glGenBuffers(1, &m_VBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(box_vertices), box_vertices,
               GL_STATIC_DRAW);

  // Create and bind EBO
  glGenBuffers(1, &m_EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(box_indices), box_indices,
               GL_STATIC_DRAW);

  // Set vertex attributes (position only)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Load shader
  try {
    m_shader = new shader("shaders/camera_test/vertex.shader",
                          "shaders/camera_test/fragment.shader");
  } catch (const std::exception &e) {
    std::cerr << "Failed to load camera test shader: " << e.what() << std::endl;
    m_shader = nullptr;
  }

  m_camera_controller = new camera_controller(m_camera, _window);
}

void camera_test_scene::render() {
  if (!m_shader) {
    return;
  }

  m_shader->use();

  glm::mat4 model = glm::mat4(1.0f);
  m_shader->set_uniform<glm::mat4, 1>("model", &model);
  m_shader->set_uniform<glm::mat4, 1>("view", &m_camera.m_view_matrix);

  // Get window dimensions from ImGui
  ImGuiIO &io = ImGui::GetIO();
  float width = io.DisplaySize.x;
  float height = io.DisplaySize.y;
  if (height == 0.0f)
    height = 1.0f; // Avoid division by zero

  m_shader->set_uniform<glm::mat4, 1>("projection",
                                      &m_camera.m_projection_matrix);

  glBindVertexArray(m_VAO);
  glDrawElements(GL_TRIANGLES, sizeof(box_indices) / sizeof(unsigned int),
                 GL_UNSIGNED_INT, 0);
}

void camera_test_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Camera Test");
  ImGui::Spacing();

  ImGui::Checkbox("Enable Camera Controller", &m_enable_camera_controller);

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

bool camera_test_scene::on_mouse_moved(double _xpos, double _ypos) {
  if (m_enable_camera_controller) {
    m_camera_controller->on_mouse_moved(_xpos, _ypos);
    return false;
  }
  return false;
}

bool camera_test_scene::on_mouse_scroll(double _xoffset, double _yoffset) {
  if (m_enable_camera_controller) {
    m_camera_controller->on_mouse_scroll(_xoffset, _yoffset);
    return false;
  }
  return false;
}

void camera_test_scene::update(float _delta_time) {
  if (m_enable_camera_controller) {
    m_camera_controller->update(_delta_time);
  }
}