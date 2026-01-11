#pragma once

#include <string>

// Base class for all test scenes
class test_scene_base {
public:
  test_scene_base(const std::string &_name);
  virtual ~test_scene_base();

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
