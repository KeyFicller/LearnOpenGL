#include "test_suit.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// ============================================================================
// TestSceneBase
// ============================================================================

TestSceneBase::TestSceneBase(const std::string &name) : m_name(name) {}

TestSceneBase::~TestSceneBase() {}

// ============================================================================
// TextureTestScene
// ============================================================================

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

TextureTestScene::TextureTestScene()
    : TestSceneBase("Texture Test"), m_VAO(0), m_VBO(0), m_EBO(0),
      m_shader(nullptr), m_texture1(nullptr), m_texture2(nullptr) {}

TextureTestScene::~TextureTestScene() {
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

void TextureTestScene::init() {
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
    m_shader = new Shader("shaders/texture_test/vertex.shader",
                          "shaders/texture_test/fragment.shader");
  } catch (const std::exception &e) {
    std::cerr << "Failed to load texture test shader: " << e.what()
              << std::endl;
    m_shader = nullptr;
  }

  // Load textures
  try {
    m_texture1 = new Texture2D("assets/images/spider_man.jpeg");
    m_texture1->set_wrap_mode(Texture2D::WrapMode::Repeat);
    m_texture1->set_filter_mode(Texture2D::FilterMode::Nearest);

    m_texture2 = new Texture2D("assets/images/sea.jpeg");
    m_texture2->set_wrap_mode(Texture2D::WrapMode::Repeat);
    m_texture2->set_filter_mode(Texture2D::FilterMode::Nearest);
  } catch (const std::exception &e) {
    std::cerr << "Failed to load texture: " << e.what() << std::endl;
  }
}

void TextureTestScene::render() {
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

void TextureTestScene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Texture Controls");
  ImGui::Spacing();
  ImGui::SliderFloat("Mix Ratio", &m_mix_ratio, 0.0f, 1.0f);
}

// ============================================================================
// TriangleTestScene
// ============================================================================

// Vertices for triangle
static float triangle_vertices[] = {
    0.0f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, // top
    0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // bottom right
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f  // bottom left
};

TriangleTestScene::TriangleTestScene()
    : TestSceneBase("Triangle Test"), m_VAO(0), m_VBO(0), m_shader(nullptr) {
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

TriangleTestScene::~TriangleTestScene() {
  if (m_VAO) {
    glDeleteVertexArrays(1, &m_VAO);
  }
  if (m_VBO) {
    glDeleteBuffers(1, &m_VBO);
  }
  delete m_shader;
}

void TriangleTestScene::init() {
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
    m_shader = new Shader("shaders/triangle_test/vertex.shader",
                          "shaders/triangle_test/fragment.shader");
  } catch (const std::exception &e) {
    std::cerr << "Failed to load triangle test shader: " << e.what()
              << std::endl;
    m_shader = nullptr;
  }

  // Initialize VBO with default data
  update_vbo();
}

void TriangleTestScene::update_vbo() {
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

void TriangleTestScene::render() {
  if (!m_shader) {
    return;
  }

  m_shader->use();
  m_shader->set_uniform<float>("ourColor", 1.0f, 1.0f, 1.0f, 1.0f);
  glBindVertexArray(m_VAO);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void TriangleTestScene::render_ui() {
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

// ============================================================================
// ColorTestScene
// ============================================================================

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

ColorTestScene::ColorTestScene()
    : TestSceneBase("Color Test"), m_VAO(0), m_VBO(0), m_EBO(0),
      m_shader(nullptr), m_animate(true) {
  m_color[0] = 1.0f; // r
  m_color[1] = 0.0f; // g
  m_color[2] = 0.0f; // b
}

ColorTestScene::~ColorTestScene() {
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

void ColorTestScene::init() {
  // Create and bind VAO
  glGenVertexArrays(1, &m_VAO);
  glBindVertexArray(m_VAO);

  // Create and bind VBO
  glGenBuffers(1, &m_VBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(color_quad_vertices),
               color_quad_vertices, GL_STATIC_DRAW);

  // Create and bind EBO
  glGenBuffers(1, &m_EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(color_quad_indices),
               color_quad_indices, GL_STATIC_DRAW);

  // Set vertex attributes (position + color, no texture)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Load shader
  try {
    m_shader = new Shader("shaders/color_test/vertex.shader",
                          "shaders/color_test/fragment.shader");
  } catch (const std::exception &e) {
    std::cerr << "Failed to load color test shader: " << e.what() << std::endl;
    m_shader = nullptr;
  }
}

void ColorTestScene::render() {
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
  glBindVertexArray(m_VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void ColorTestScene::render_ui() {
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

// ============================================================================
// TransformTestScene
// ============================================================================

TransformTestScene::TransformTestScene()
    : TestSceneBase("Transform Test"), m_VAO(0), m_VBO(0), m_shader(nullptr) {}

TransformTestScene::~TransformTestScene() {
  if (m_VAO) {
    glDeleteVertexArrays(1, &m_VAO);
  }
  if (m_VBO) {
    glDeleteBuffers(1, &m_VBO);
  }
  delete m_shader;
}

void TransformTestScene::init() {
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
    m_shader = new Shader("shaders/transform_test/vertex.shader",
                          "shaders/transform_test/fragment.shader");
  } catch (const std::exception &e) {
    std::cerr << "Failed to load transform test shader: " << e.what()
              << std::endl;
    m_shader = nullptr;
  }
}

void TransformTestScene::render() {
  if (!m_shader) {
    return;
  }

  float time = glfwGetTime();
  float angle = time * glm::radians(m_rotation_speed);
  m_transform = glm::rotate(m_transform, angle, m_rotation_axis);

  m_shader->use();
  m_shader->set_uniform<glm::mat4>("transform", m_transform);
  glBindVertexArray(m_VAO);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}

void TransformTestScene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Transform Controls");
  ImGui::Spacing();

  ImGui::SliderFloat("Rotation Speed", &m_rotation_speed, 0.0f, 10.0f);
  ImGui::SliderFloat3("Rotation Axis", &m_rotation_axis[0], -1.0f, 1.0f);

  ImGui::Spacing();
}

// ============================================================================
// TestSuit
// ============================================================================

TestSuit::TestSuit() : m_current_scene(TestScene::TextureTest) {}

TestSuit::~TestSuit() {
  for (auto &[scene, test_scene] : m_scenes) {
    delete test_scene;
  }
}

void TestSuit::init() {
  // Create and initialize all test scenes
  m_scenes[TestScene::TextureTest] = new TextureTestScene();
  m_scenes[TestScene::TextureTest]->init();

  m_scenes[TestScene::TriangleTest] = new TriangleTestScene();
  m_scenes[TestScene::TriangleTest]->init();

  m_scenes[TestScene::ColorTest] = new ColorTestScene();
  m_scenes[TestScene::ColorTest]->init();

  m_scenes[TestScene::TransformTest] = new TransformTestScene();
  m_scenes[TestScene::TransformTest]->init();
}

void TestSuit::render_ui() {
  ImGui::Begin("Test Scene Control");

  ImGui::Text("Select Test Scene:");
  ImGui::Separator();

  for (int i = 0; i < static_cast<int>(TestScene::COUNT); i++) {
    TestScene scene = static_cast<TestScene>(i);
    bool is_selected = (m_current_scene == scene);
    if (ImGui::RadioButton(get_scene_name(scene), is_selected)) {
      m_current_scene = scene;
    }
  }

  ImGui::Separator();
  TestSceneBase *current = get_scene(m_current_scene);
  if (current) {
    ImGui::Text("Current Scene: %s", current->get_name());

    // Render scene-specific UI controls
    current->render_ui();
  }
  ImGui::Separator();
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

  ImGui::End();
}

void TestSuit::render_scene() {
  TestSceneBase *scene = get_scene(m_current_scene);
  if (scene) {
    scene->render();
  }
}

const char *TestSuit::get_scene_name(TestScene scene) const {
  switch (scene) {
  case TestScene::TextureTest:
    return "Texture Test";
  case TestScene::TriangleTest:
    return "Triangle Test";
  case TestScene::ColorTest:
    return "Color Test";
  case TestScene::TransformTest:
    return "Transform Test";
  default:
    return "Unknown";
  }
}

TestSceneBase *TestSuit::get_scene(TestScene scene) { return m_scenes[scene]; }
