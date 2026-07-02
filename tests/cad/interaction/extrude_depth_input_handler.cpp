#include "extrude_depth_input_handler.h"

#include "ray_pick.h"
#include "command_dispatcher.h"
#include "tests/cad/instance.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

namespace toy_cad::interaction {

void extrude_depth_input_handler::set_extrude_frame(const glm::vec3 &anchor,
                                                    const glm::vec3 &direction) {
  m_anchor = anchor;
  m_direction = glm::length(direction) > 1e-6f ? glm::normalize(direction)
                                               : glm::vec3(0.0f, 0.0f, 1.0f);
  m_depth = 0.0f;
  m_valid = false;
}

bool extrude_depth_input_handler::on_mouse_moved(double xpos, double ypos) {
  update_depth(xpos, ypos);

  if (owner()) {
    owner()->on_input_changed(flag(), this, false);
  }

  return true;
}

bool extrude_depth_input_handler::on_mouse_button(int button, int action,
                                                  int mods) {
  (void)mods;

  if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
    return false;
  }

  if (!m_valid) {
    return false;
  }

  if (owner()) {
    owner()->on_input_changed(flag(), this, true);
  }

  return true;
}

void extrude_depth_input_handler::update_depth(double xpos, double ypos) {
  auto &disp = instance::get().disp();
  const glm::vec3 camera_pos = disp.camera_world_position;

  glm::vec3 view_dir = camera_pos - m_anchor;
  if (glm::length(view_dir) < 1e-6f) {
    view_dir = glm::vec3(0.0f, 0.0f, 1.0f);
  } else {
    view_dir = glm::normalize(view_dir);
  }

  glm::vec3 plane_normal = glm::cross(m_direction, view_dir);
  if (glm::length(plane_normal) < 1e-6f) {
    plane_normal = glm::cross(m_direction, glm::vec3(0.0f, 1.0f, 0.0f));
  }
  if (glm::length(plane_normal) < 1e-6f) {
    plane_normal = glm::cross(m_direction, glm::vec3(1.0f, 0.0f, 0.0f));
  }
  plane_normal = glm::normalize(plane_normal);

  glm::vec3 picked{0.0f};
  m_valid = ray_pick::instance().pick_on_plane(
      xpos, ypos, disp, m_anchor, plane_normal, picked);

  if (!m_valid) {
    return;
  }

  m_depth = glm::dot(picked - m_anchor, m_direction);
}

} // namespace toy_cad::interaction
