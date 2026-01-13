#pragma once

#include "tests/scenes/scene_base.h"
#include <map>

// Test scene enumeration
enum class test_scene {
  k_texture_test,
  k_triangle_test,
  k_color_test,
  k_transform_test,
  k_coordinate_test,
  k_camera_test,
  k_light_color_test,
  k_count
};

// Test suit class for managing test scenarios
class test_suit {
public:
  test_suit();
  ~test_suit();

  // Initialize test resources
  void init(GLFWwindow *_window);

  // Update the test suit
  void update(float _delta_time);

  // Render ImGui control panel
  void render_ui();

  // Render current test scene
  void render_scene();

  // Get current scene name
  const char *get_scene_name(test_scene _scene);

  // Event handlers
  bool on_mouse_moved(double _xpos, double _ypos);
  bool on_mouse_scroll(double _xoffset, double _yoffset);

private:
  // Current active test scene
  test_scene m_current_scene;

  // Test scenes
  std::map<test_scene, test_scene_base *> m_scenes;

  // Get scene instance by enum
  test_scene_base *get_scene(test_scene _scene);
};
