#include "test_suit.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <cmath>
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
    : TestSceneBase("Triangle Test"), m_VAO(0), m_VBO(0), m_shader(nullptr) {}

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
    m_shader = new Shader("shaders/triangle_test/vertex.shader",
                          "shaders/triangle_test/fragment.shader");
  } catch (const std::exception &e) {
    std::cerr << "Failed to load triangle test shader: " << e.what()
              << std::endl;
    m_shader = nullptr;
  }
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
      m_shader(nullptr) {}

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
  float time = glfwGetTime();
  float r = (sin(time) + 1.0f) * 0.5f;
  float g = (sin(time + 2.0f) + 1.0f) * 0.5f;
  float b = (sin(time + 4.0f) + 1.0f) * 0.5f;
  m_shader->set_uniform<float>("ourColor", r, g, b, 1.0f);
  glBindVertexArray(m_VAO);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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
  }
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
  default:
    return "Unknown";
  }
}

TestSceneBase *TestSuit::get_scene(TestScene scene) { return m_scenes[scene]; }
