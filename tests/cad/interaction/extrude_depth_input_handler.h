#pragma once

#include "input_handler.h"
#include <glm/glm.hpp>

namespace toy_cad::interaction {

/**
 * Drag along an extrusion axis to set signed depth.
 * Uses a view-facing plane through the anchor so mouse motion maps to height.
 */
class extrude_depth_input_handler : public input_handler {
public:
  extrude_depth_input_handler() = default;

  extrude_depth_input_handler(const extrude_depth_input_handler &) = delete;
  extrude_depth_input_handler &
  operator=(const extrude_depth_input_handler &) = delete;
  extrude_depth_input_handler(extrude_depth_input_handler &&) noexcept =
      default;
  extrude_depth_input_handler &
  operator=(extrude_depth_input_handler &&) noexcept = default;

  ~extrude_depth_input_handler() override = default;

  void set_extrude_frame(const glm::vec3 &anchor, const glm::vec3 &direction);

  bool on_mouse_moved(double xpos, double ypos) override;
  bool on_mouse_button(int button, int action, int mods) override;

  [[nodiscard]] float preview_depth() const { return m_depth; }
  [[nodiscard]] bool has_valid_preview() const { return m_valid; }

private:
  glm::vec3 m_anchor{0.0f};
  glm::vec3 m_direction{0.0f, 0.0f, 1.0f};
  float m_depth = 0.0f;
  bool m_valid = false;

  void update_depth(double xpos, double ypos);
};

} // namespace toy_cad::interaction
