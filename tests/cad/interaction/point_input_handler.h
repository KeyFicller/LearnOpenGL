#pragma once

#include "input_handler.h"
#include "tests/cad/renderer/disp_var.h"
#include <glm/glm.hpp>

namespace toy_cad::interaction {

/**
 * Input handler for 3D point picking.
 *
 * Usage:
 *   1. Create and set owner command via set_owner(command, "flag")
 *   2. Push onto input stack
 *   3. Handler calls owner->on_input_changed(flag, this) on mouse move and click
 *   4. Command extracts preview_point() / has_valid_preview() from handler
 */
class point_input_handler : public input_handler {
public:
  point_input_handler() = default;

  point_input_handler(const point_input_handler &) = delete;
  point_input_handler &operator=(const point_input_handler &) = delete;
  point_input_handler(point_input_handler &&) noexcept = default;
  point_input_handler &operator=(point_input_handler &&) noexcept = default;

  ~point_input_handler() override = default;

  bool on_mouse_moved(double xpos, double ypos) override;
  bool on_mouse_button(int button, int action, int mods) override;

  /** Get last preview point (may be invalid if ray parallel to plane). */
  [[nodiscard]] glm::vec3 preview_point() const { return m_preview_point; }
  [[nodiscard]] bool has_valid_preview() const { return m_preview_valid; }

private:
  glm::vec3 m_preview_point{0.0f};
  bool m_preview_valid = false;

  void update_preview(double xpos, double ypos);
};

} // namespace toy_cad::interaction
