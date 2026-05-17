#pragma once

#include <glm/glm.hpp>

namespace toy_cad::interaction {

/**
 * Debug panel for CAD interaction diagnostics.
 * Displays ray pick coordinates, working plane info, and other debug state.
 */
class debug_panel {
public:
  static debug_panel &instance();

  debug_panel(const debug_panel &) = delete;
  debug_panel &operator=(const debug_panel &) = delete;
  debug_panel(debug_panel &&) noexcept = default;
  debug_panel &operator=(debug_panel &&) noexcept = default;

  /** Draw the debug panel UI (call inside ImGui context). */
  void draw_ui();

  /** Update cached ray pick coordinate from default working plane. */
  void set_ray_pick_coordinate(const glm::vec3 &coord, bool valid);

  /** Set current mouse position for display. */
  void set_mouse_position(double x, double y);

private:
  debug_panel() = default;
  ~debug_panel() = default;

  glm::vec3 m_ray_pick_coord{0.0f, 0.0f, 0.0f};
  bool m_ray_pick_valid = false;
  double m_mouse_x = 0.0;
  double m_mouse_y = 0.0;
};

} // namespace toy_cad::interaction
