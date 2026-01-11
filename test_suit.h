#pragma once

#include "shader.h"
#include "texture.h"
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <map>
#include <string>

// Base class for all test scenes
class TestSceneBase {
public:
  TestSceneBase(const std::string &name);
  virtual ~TestSceneBase();

  // Initialize scene resources
  virtual void init() = 0;

  // Render the scene
  virtual void render() = 0;

  // Render scene-specific UI controls
  virtual void render_ui() {}

  // Get scene name
  const char *get_name() const { return m_name.c_str(); }

protected:
  std::string m_name;
};

// Texture test scene
class TextureTestScene : public TestSceneBase {
public:
  TextureTestScene();
  virtual ~TextureTestScene();

  void init() override;
  void render() override;
  void render_ui() override;

private:
  GLuint m_VAO;
  GLuint m_VBO;
  GLuint m_EBO;
  Shader *m_shader;
  Texture2D *m_texture1;
  Texture2D *m_texture2;
  float m_mix_ratio; // Texture mixing ratio
};

// Triangle test scene
class TriangleTestScene : public TestSceneBase {
public:
  TriangleTestScene();
  virtual ~TriangleTestScene();

  void init() override;
  void render() override;
  void render_ui() override;

private:
  GLuint m_VAO;
  GLuint m_VBO;
  Shader *m_shader;
  // Triangle vertex positions (x, y for each vertex)
  float m_vertices[3][2]; // [vertex_index][x, y]
  // Triangle vertex colors (r, g, b for each vertex)
  float m_colors[3][3]; // [vertex_index][r, g, b]

  // Update VBO with current vertex data
  void update_vbo();
};

// Color test scene
class ColorTestScene : public TestSceneBase {
public:
  ColorTestScene();
  virtual ~ColorTestScene();

  void init() override;
  void render() override;
  void render_ui() override;

private:
  GLuint m_VAO;
  GLuint m_VBO;
  GLuint m_EBO;
  Shader *m_shader;
  // Color control (RGB)
  float m_color[3];
  // Animation toggle
  bool m_animate;
};

// Transform test scene
class TransformTestScene : public TestSceneBase {
public:
  TransformTestScene();
  virtual ~TransformTestScene();

  void init() override;
  void render() override;
  void render_ui() override;

private:
  glm::mat4 m_transform = glm::mat4(1.0f);
  GLuint m_VAO;
  GLuint m_VBO;
  Shader *m_shader;
  float m_rotation_speed = 1.0f; // Rotation speed in degrees per second
  glm::vec3 m_rotation_axis = glm::vec3(0.0f, 0.0f, 1.0f); // Rotation axis
};

// Test scene enumeration
enum class TestScene {
  TextureTest,   // Texture mixing test
  TriangleTest,  // Simple triangle test
  ColorTest,     // Color test
  TransformTest, // Transform test
  COUNT          // Total number of scenes
};

// Test suit class for managing test scenarios
class TestSuit {
public:
  TestSuit();
  ~TestSuit();

  // Initialize test resources
  void init();

  // Render ImGui control panel
  void render_ui();

  // Render current test scene
  void render_scene();

  // Get current scene name
  const char *get_scene_name(TestScene scene) const;

private:
  // Current active test scene
  TestScene m_current_scene;

  // Test scenes
  std::map<TestScene, TestSceneBase *> m_scenes;

  // Get scene instance by enum
  TestSceneBase *get_scene(TestScene scene);
};
