#pragma once

#include "basic/shader.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/sub_scene.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

// Forward declaration
class spline_movement_scene;

/**
 * @brief Sub-scene: Spline Movement - Snake
 */
class spline_movement_snake_sub_scene
    : public sub_scene<spline_movement_scene> {
public:
  spline_movement_snake_sub_scene(spline_movement_scene *_parent);
  ~spline_movement_snake_sub_scene() override;

  void render() override;
  void render_ui() override;
  void update(float _delta_time) override;
  bool on_mouse_moved(double _xpos, double _ypos) override;

  void update_mesh_data();

private:
  mesh_manager m_points_mesh_manager;
  mesh_manager m_line_strip_mesh_manager;
  shader *m_shader = nullptr;
  shader *m_line_strip_shader = nullptr;
  std::vector<glm::vec3> m_points;
  int m_point_count = 20;
  float m_segment_length = 0.06f;
  float m_update_frequency = 0.033f; // 30fps
  float m_update_timer = 0.0f;
};