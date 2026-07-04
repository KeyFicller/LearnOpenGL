#include "extrude_depth_input_handler.h"

#include "ray_pick.h"
#include "command_dispatcher.h"
#include "tests/cad/instance.h"
#include "tests/cad/renderer/disp_var.h"
#include "tests/framework/test_suit.h"

#include <GLFW/glfw3.h>
#include <cmath>

namespace toy_cad::interaction {

namespace {

glm::vec2 viewport_relative_cursor() {
  GLFWwindow *window = instance::get().window();
  if (!window) {
    return glm::vec2(0.0f);
  }
  double x = 0.0;
  double y = 0.0;
  glfwGetCursorPos(window, &x, &y);
  if (auto *test_suit_ptr =
          static_cast<test_suit *>(glfwGetWindowUserPointer(window))) {
    x -= test_suit_ptr->m_viewport_x;
    y -= test_suit_ptr->m_viewport_y;
  }
  return glm::vec2(static_cast<float>(x), static_cast<float>(y));
}

glm::vec3 camera_forward(const disp_var &disp) {
  glm::vec3 forward = -glm::normalize(glm::vec3(disp.view_matrix[0][2],
                                                disp.view_matrix[1][2],
                                                disp.view_matrix[2][2]));
  if (!std::isfinite(forward.x)) {
    forward = glm::vec3(0.0f, 0.0f, 1.0f);
  }
  return forward;
}

glm::vec3 extrude_drag_plane_normal(const glm::vec3 &direction,
                                    const glm::vec3 &view_dir) {
  glm::vec3 plane_normal = glm::cross(direction, view_dir);
  if (glm::length(plane_normal) < 1e-6f) {
    plane_normal = glm::cross(direction, glm::vec3(0.0f, 0.0f, 1.0f));
  }
  if (glm::length(plane_normal) < 1e-6f) {
    plane_normal = glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f));
  }
  if (glm::length(plane_normal) < 1e-6f) {
    plane_normal = glm::cross(direction, glm::vec3(1.0f, 0.0f, 0.0f));
  }
  return glm::normalize(plane_normal);
}

} // namespace

void extrude_depth_input_handler::set_extrude_frame(const glm::vec3 &anchor,
                                                    const glm::vec3 &direction) {
  m_anchor = anchor;
  m_direction = glm::length(direction) > 1e-6f ? glm::normalize(direction)
                                               : glm::vec3(0.0f, 0.0f, 1.0f);
  m_depth = 0.0f;
  m_valid = false;
}

void extrude_depth_input_handler::sync_from_viewport_cursor() {
  const glm::vec2 cursor = viewport_relative_cursor();
  update_depth(cursor.x, cursor.y);
  if (owner()) {
    owner()->on_input_changed(flag(), this, false);
  }
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

  const glm::vec2 cursor = viewport_relative_cursor();
  update_depth(cursor.x, cursor.y);

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
  const auto ray = ray_pick::instance().screen_to_world_ray(xpos, ypos, disp);
  const glm::vec3 view_dir = camera_forward(disp);
  const glm::vec3 plane_normal =
      extrude_drag_plane_normal(m_direction, view_dir);

  glm::vec3 picked{0.0f};
  if (ray_pick::instance().pick_on_plane(
          xpos, ypos, disp, m_anchor, plane_normal, picked)) {
    m_depth = glm::dot(picked - m_anchor, m_direction);
    m_valid = true;
    return;
  }

  // Ray parallel to drag plane: project mouse ray onto the extrusion axis.
  const glm::vec3 oc = ray.origin - m_anchor;
  const float dir_dot_ray = glm::dot(m_direction, ray.direction);
  const float denom = 1.0f - dir_dot_ray * dir_dot_ray;
  if (std::abs(denom) >= 1e-6f) {
    m_depth = (glm::dot(oc, m_direction) -
               dir_dot_ray * glm::dot(oc, ray.direction)) /
              denom;
    m_valid = true;
    return;
  }

  m_valid = false;
}

} // namespace toy_cad::interaction
