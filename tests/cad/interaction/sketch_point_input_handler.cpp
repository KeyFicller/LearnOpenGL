#include "sketch_point_input_handler.h"

#include "command_dispatcher.h"
#include "ray_pick.h"
#include "tests/cad/interaction/sketch_context.h"
#include "tests/cad/instance.h"
#include "tests/framework/test_suit.h"

#include <gp_Ax2.hxx>
#include <gp_Pln.hxx>

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

glm::vec3 to_glm(const gp_Pnt &p) {
  return glm::vec3(static_cast<float>(p.X()), static_cast<float>(p.Y()),
                   static_cast<float>(p.Z()));
}

glm::vec3 to_glm(const gp_Dir &d) {
  return glm::vec3(static_cast<float>(d.X()), static_cast<float>(d.Y()),
                   static_cast<float>(d.Z()));
}

bool pick_on_sketch_plane(double xpos, double ypos, const disp_var &disp,
                          const gp_Ax2 &ax2, glm::vec3 &out_point) {
  const gp_Pln pln(ax2);
  const glm::vec3 origin = to_glm(pln.Location());
  const glm::vec3 normal = to_glm(pln.Axis().Direction());

  if (ray_pick::instance().pick_on_plane(xpos, ypos, disp, origin, normal,
                                         out_point)) {
    return true;
  }

  const auto ray = ray_pick::instance().screen_to_world_ray(xpos, ypos, disp);
  const float denom = glm::dot(ray.direction, normal);
  if (std::abs(denom) < 1e-6f) {
    out_point = ray.origin - normal * glm::dot(ray.origin - origin, normal);
    return true;
  }

  const float t = glm::dot(origin - ray.origin, normal) / denom;
  out_point = ray.origin + ray.direction * t;
  return std::isfinite(out_point.x) && std::isfinite(out_point.y) &&
         std::isfinite(out_point.z);
}

} // namespace

void sketch_point_input_handler::sync_from_viewport_cursor() {
  const glm::vec2 cursor = viewport_relative_cursor();
  update_preview(cursor.x, cursor.y);
  if (owner()) {
    owner()->on_input_changed(flag(), this, false);
  }
}

bool sketch_point_input_handler::on_mouse_moved(double xpos, double ypos) {
  update_preview(xpos, ypos);

  if (owner()) {
    owner()->on_input_changed(flag(), this, false);
  }

  return true;
}

bool sketch_point_input_handler::on_mouse_button(int button, int action,
                                                 int mods) {
  (void)mods;

  if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
    return false;
  }

  const glm::vec2 cursor = viewport_relative_cursor();
  update_preview(cursor.x, cursor.y);

  if (!m_preview_valid) {
    return false;
  }

  if (owner()) {
    owner()->on_input_changed(flag(), this, true);
  }

  return true;
}

void sketch_point_input_handler::update_preview(double xpos, double ypos) {
  if (!sketch_context::instance().is_active()) {
    m_preview_valid = false;
    return;
  }

  glm::vec3 world_point;
  m_preview_valid = pick_on_sketch_plane(
      xpos, ypos, instance::get().disp(), sketch_context::instance().plane(),
      world_point);

  if (m_preview_valid) {
    m_preview_point = world_point;
  }
}

} // namespace toy_cad::interaction
