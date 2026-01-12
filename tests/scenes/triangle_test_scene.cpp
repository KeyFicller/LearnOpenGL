#include "triangle_test_scene.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <iostream>

triangle_test_scene::triangle_test_scene()
    : test_scene_base("Triangle Test"), m_VAO(0), m_VBO(0), m_shader(nullptr) {
  // Initialize default vertex positions
  m_vertices[0][0] = 0.0f;  // top x
  m_vertices[0][1] = 0.5f;  // top y
  m_vertices[1][0] = 0.5f;  // bottom right x
  m_vertices[1][1] = -0.5f; // bottom right y
  m_vertices[2][0] = -0.5f; // bottom left x
  m_vertices[2][1] = -0.5f; // bottom left y

  // Initialize default vertex colors
  m_colors[0][0] = 1.0f; // top red
  m_colors[0][1] = 0.0f; // top green
  m_colors[0][2] = 0.0f; // top blue
  m_colors[1][0] = 0.0f; // bottom right red
  m_colors[1][1] = 1.0f; // bottom right green
  m_colors[1][2] = 0.0f; // bottom right blue
  m_colors[2][0] = 0.0f; // bottom left red
  m_colors[2][1] = 0.0f; // bottom left green
  m_colors[2][2] = 1.0f; // bottom left blue
}

triangle_test_scene::~triangle_test_scene() {
  if (m_VAO) {
    glDeleteVertexArrays(1, &m_VAO);
  }
  if (m_VBO) {
    glDeleteBuffers(1, &m_VBO);
  }
  delete m_shader;
}

void triangle_test_scene::init(GLFWwindow *_window) {
  test_scene_base::init(_window);
  // Create and bind VAO
  glGenVertexArrays(1, &m_VAO);
  glBindVertexArray(m_VAO);

  // Create and bind VBO
  glGenBuffers(1, &m_VBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

  // Set vertex attributes (position + color, no texture)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Load shader
  try {
    m_shader = new shader("shaders/triangle_test/vertex.shader",
                          "shaders/triangle_test/fragment.shader");
  } catch (const std::exception &e) {
    std::cerr << "Failed to load triangle test shader: " << e.what()
              << std::endl;
    m_shader = nullptr;
  }

  // Initialize VBO with default data
  update_vbo();
}

void triangle_test_scene::update_vbo() {
  // Build vertex data array from m_vertices and m_colors
  float vertex_data[3][6]; // 3 vertices, each with 6 floats (x, y, z, r, g, b)
  for (int i = 0; i < 3; i++) {
    vertex_data[i][0] = m_vertices[i][0]; // x
    vertex_data[i][1] = m_vertices[i][1]; // y
    vertex_data[i][2] = 0.0f;             // z
    vertex_data[i][3] = m_colors[i][0];   // r
    vertex_data[i][4] = m_colors[i][1];   // g
    vertex_data[i][5] = m_colors[i][2];   // b
  }

  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data,
               GL_DYNAMIC_DRAW);
}

void triangle_test_scene::render() {
  if (!m_shader) {
    return;
  }

  m_shader->use();
  m_shader->set_uniform<float>("ourColor", 1.0f, 1.0f, 1.0f, 1.0f);
  glBindVertexArray(m_VAO);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void triangle_test_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Triangle Controls");
  ImGui::Spacing();

  bool vertices_changed = false;
  bool colors_changed = false;

  // Vertex position controls
  ImGui::Text("Vertex Positions:");
  for (int i = 0; i < 3; i++) {
    const char *labels[] = {"Top", "Bottom Right", "Bottom Left"};
    ImGui::PushID(i);
    if (ImGui::DragFloat2(labels[i], m_vertices[i], 0.01f, -1.0f, 1.0f)) {
      vertices_changed = true;
    }
    ImGui::PopID();
  }

  ImGui::Spacing();

  // Vertex color controls
  ImGui::Text("Vertex Colors:");
  for (int i = 0; i < 3; i++) {
    const char *labels[] = {"Top", "Bottom Right", "Bottom Left"};
    ImGui::PushID(i + 10);
    if (ImGui::ColorEdit3(labels[i], m_colors[i])) {
      colors_changed = true;
    }
    ImGui::PopID();
  }

  // Update VBO if vertices or colors changed
  if (vertices_changed || colors_changed) {
    update_vbo();
  }
}
