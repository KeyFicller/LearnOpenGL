#pragma once

#include "shader.h"
#include "texture.h"
#include <glad/gl.h>
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

private:
  GLuint m_VAO;
  GLuint m_VBO;
  GLuint m_EBO;
  Shader *m_shader;
  Texture2D *m_texture1;
  Texture2D *m_texture2;
};

// Triangle test scene
class TriangleTestScene : public TestSceneBase {
public:
  TriangleTestScene();
  virtual ~TriangleTestScene();

  void init() override;
  void render() override;

private:
  GLuint m_VAO;
  GLuint m_VBO;
  Shader *m_shader;
};

// Color test scene
class ColorTestScene : public TestSceneBase {
public:
  ColorTestScene();
  virtual ~ColorTestScene();

  void init() override;
  void render() override;

private:
  GLuint m_VAO;
  GLuint m_VBO;
  GLuint m_EBO;
  Shader *m_shader;
};

// Test scene enumeration
enum class TestScene {
  TextureTest,  // Texture mixing test
  TriangleTest, // Simple triangle test
  ColorTest,    // Color test
  COUNT         // Total number of scenes
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
