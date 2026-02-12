#pragma once

#include "periscope.h"
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
  k_light_material_test,
  k_light_texture_test,
  k_light_type_test,
  k_multiple_light_test,
  k_import_model_test,
  k_depth_test_test,
  k_stencil_test_test,
  k_blender_test_test,
  k_cull_test_test,
  k_texture_cube_test_test,
  k_advanced_glsl_test_test,
  k_geometry_shader_test_test,
  k_instance_test,
  k_basic_test_count,
  k_spline_movement_test,
  k_advanced_test_count,
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
  void on_framebuffer_resized(int _width, int _height);

public:
  // Cached viewport window information
  float m_viewport_x = 0.0f;
  float m_viewport_y = 0.0f;
  float m_viewport_width = 0.0f;
  float m_viewport_height = 0.0f;

private:
  // Current active test scene
  test_scene m_current_scene;

  // Test scenes
  std::map<test_scene, test_scene_base *> m_scenes;

  // Get scene instance by enum
  test_scene_base *get_scene(test_scene _scene);
};
