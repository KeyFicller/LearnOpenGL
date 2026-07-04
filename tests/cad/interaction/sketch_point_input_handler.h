#pragma once

#include "input_handler.h"
#include <glm/glm.hpp>

namespace toy_cad::interaction {

/** Pick 2D points on the active sketch plane. */
class sketch_point_input_handler : public input_handler {
public:
  sketch_point_input_handler() = default;

  sketch_point_input_handler(const sketch_point_input_handler &) = delete;
  sketch_point_input_handler &
  operator=(const sketch_point_input_handler &) = delete;
  sketch_point_input_handler(sketch_point_input_handler &&) noexcept = default;
  sketch_point_input_handler &
  operator=(sketch_point_input_handler &&) noexcept = default;

  ~sketch_point_input_handler() override = default;

  bool on_mouse_moved(double xpos, double ypos) override;
  bool on_mouse_button(int button, int action, int mods) override;

  [[nodiscard]] glm::vec3 preview_point() const { return m_preview_point; }
  [[nodiscard]] bool has_valid_preview() const { return m_preview_valid; }

  /** Refresh preview from current viewport cursor (call after pushing handler). */
  void sync_from_viewport_cursor();

private:
  glm::vec3 m_preview_point{0.0f};
  bool m_preview_valid = false;

  void update_preview(double xpos, double ypos);
};

} // namespace toy_cad::interaction
