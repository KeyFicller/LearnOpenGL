#pragma once

#include "input_handler.h"
#include "tests/cad/database/handle.h"
#include <glm/glm.hpp>
#include <optional>

namespace toy_cad::interaction {

/**
 * Input handler for picking a brep face via ray casting.
 * Ignores edges; notifies owner on hover (preview) and click (confirm).
 */
class face_input_handler : public input_handler {
public:
  face_input_handler() = default;

  face_input_handler(const face_input_handler &) = delete;
  face_input_handler &operator=(const face_input_handler &) = delete;
  face_input_handler(face_input_handler &&) noexcept = default;
  face_input_handler &operator=(face_input_handler &&) noexcept = default;

  ~face_input_handler() override = default;

  bool on_mouse_moved(double xpos, double ypos) override;
  bool on_mouse_button(int button, int action, int mods) override;

  [[nodiscard]] std::optional<handle> hover_face() const { return m_hover_face; }
  [[nodiscard]] std::optional<glm::vec3> hover_point() const { return m_hover_point; }
  [[nodiscard]] bool has_valid_hover() const {
    return m_hover_face.has_value() && m_hover_point.has_value();
  }

private:
  std::optional<handle> m_hover_face;
  std::optional<glm::vec3> m_hover_point;

  void update_hover(double xpos, double ypos);
};

} // namespace toy_cad::interaction
