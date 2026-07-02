#include "face_input_handler.h"

#include "ray_pick.h"
#include "selection_manager.h"
#include "tests/cad/database/brep/brep.h"
#include "tests/cad/database/brep/brep_query.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/command_dispatcher.h"

#include <GLFW/glfw3.h>
#include <limits>

namespace toy_cad::interaction {

namespace {

std::optional<std::pair<handle, glm::vec3>>
ray_cast_face(const glm::vec3 &ray_origin, const glm::vec3 &ray_dir) {
  auto &db = instance::get().db();

  float closest_t = std::numeric_limits<float>::max();
  std::optional<handle> closest_face;
  glm::vec3 closest_point{0.0f};

  for_each_scene_brep([&](handle /*brep_h*/, brep &brep_obj) {
    if (!brep_obj.has_bvh()) {
      return;
    }

    const auto hit = brep_obj.intersect_ray(db, ray_origin, ray_dir);
    if (!hit.has_value() || hit->t <= 0.0f || hit->t >= closest_t) {
      return;
    }

    if (subshape_kind(db, hit->subshape) != brep_subshape_kind::face) {
      return;
    }

    closest_t = hit->t;
    closest_face = hit->subshape;
    closest_point = ray_origin + ray_dir * hit->t;
  });

  if (!closest_face.has_value()) {
    return std::nullopt;
  }
  return std::make_pair(*closest_face, closest_point);
}

} // namespace

bool face_input_handler::on_mouse_moved(double xpos, double ypos) {
  update_hover(xpos, ypos);

  if (owner()) {
    owner()->on_input_changed(flag(), this, false);
  }

  return true;
}

bool face_input_handler::on_mouse_button(int button, int action, int mods) {
  (void)mods;

  if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
    return false;
  }

  if (!has_valid_hover()) {
    return false;
  }

  if (owner()) {
    owner()->on_input_changed(flag(), this, true);
  }

  return true;
}

void face_input_handler::update_hover(double xpos, double ypos) {
  auto &disp = instance::get().disp();
  const auto ray = ray_pick::instance().screen_to_world_ray(xpos, ypos, disp);

  const auto hit = ray_cast_face(ray.origin, ray.direction);
  if (!hit.has_value()) {
    m_hover_face.reset();
    m_hover_point.reset();
    selection_manager::instance().clear_hover_target();
    return;
  }

  m_hover_face = hit->first;
  m_hover_point = hit->second;
  selection_manager::instance().set_hover_target(subshape_ref{*m_hover_face});
}

} // namespace toy_cad::interaction
