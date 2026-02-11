#pragma once

#include "basic/shader.h"
#include "spline_movement_driver.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/sub_scene.h"
#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <string>

// Forward declaration
class spline_movement_scene;

enum class spline_shader_type {
  k_control_points,
  k_spline,
  k_head,
  k_attachment
};

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
  void draw_snake();

  virtual void update_mesh_data();

protected:
  mesh_manager m_points_mesh_manager;
  mesh_manager m_line_strip_mesh_manager;
  std::map<spline_shader_type, shader *> m_shaders;
  std::vector<glm::vec3> m_smooth_points;
  bool m_draw_control_points = true;
  spline_driving::spline m_snake_spline = spline_driving::spline(
      glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 10);
};

/**
 * @brief Sub-scene: Spline Movement - Lizard
 */
class spline_movement_lizard_sub_scene
    : public spline_movement_snake_sub_scene {
public:
  spline_movement_lizard_sub_scene(spline_movement_scene *_parent);
  ~spline_movement_lizard_sub_scene() override;

  void render() override;

  void update_mesh_data() override;

  void render_ui() override;

protected:
  mesh_manager m_legs_control_points_manager[4];
  mesh_manager m_legs_line_strip_manager[4];
};