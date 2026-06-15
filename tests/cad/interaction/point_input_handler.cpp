#include "point_input_handler.h"

#include "ray_pick.h"
#include "command_dispatcher.h"
#include "tests/cad/instance.h"
#include <GLFW/glfw3.h>

namespace toy_cad::interaction {

bool point_input_handler::on_mouse_moved(double xpos, double ypos) {
  update_preview(xpos, ypos);

  // Notify owner command (preview mode: confirmed = false)
  if (owner()) {
    owner()->on_input_changed(flag(), this, false);
  }

  return true; // Consume the event
}

bool point_input_handler::on_mouse_button(int button, int action, int mods) {
  (void)mods;

  if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
    return false;
  }

  if (!m_preview_valid) {
    return false; // No valid point to confirm
  }

  // Notify owner command (confirmation mode: confirmed = true)
  if (owner()) {
    owner()->on_input_changed(flag(), this, true);
  }

  return true;
}

void point_input_handler::update_preview(double xpos, double ypos) {
  auto &picker = ray_pick::instance();
  glm::vec3 world_point;

  m_preview_valid = picker.pick_at(
      xpos, ypos,
      instance::get().disp(),
      world_point);

  if (m_preview_valid) {
    m_preview_point = world_point;
  }
}

} // namespace toy_cad::interaction
