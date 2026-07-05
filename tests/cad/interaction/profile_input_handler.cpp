#include "profile_input_handler.h"

#include "ray_pick.h"
#include "selection_manager.h"
#include "sketch_curve_pick.h"
#include "tests/cad/database/brep/brep.h"
#include "tests/cad/database/brep/brep_query.h"
#include "tests/cad/geometry/occt_mesh_utils.h"
#include "tests/cad/history/sketch_feature.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/command_dispatcher.h"

#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <GLFW/glfw3.h>
#include <cmath>
#include <limits>

namespace toy_cad::interaction {

namespace {

constexpr float k_sketch_curve_pick_px = 10.0f;

glm::vec3 to_glm(const gp_Pnt &p) {
  return glm::vec3(static_cast<float>(p.X()), static_cast<float>(p.Y()),
                   static_cast<float>(p.Z()));
}

glm::vec3 to_glm(const gp_Dir &d) {
  return glm::vec3(static_cast<float>(d.X()), static_cast<float>(d.Y()),
                   static_cast<float>(d.Z()));
}

const sketch_feature *find_sketch_for_brep(database &db, handle brep_h) {
  if (!brep_h.valid()) {
    return nullptr;
  }
  for (const handle feature_h : instance::get().history()) {
    const auto *sketch = db.try_get_as<sketch_feature>(feature_h);
    if (sketch && sketch->brep_handle() == brep_h && sketch->has_profile()) {
      return sketch;
    }
  }
  return nullptr;
}

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

bool pick_on_sketch_plane(double xpos, double ypos, const disp_var &disp,
                          const gp_Ax2 &ax2, glm::vec3 &out_point) {
  const gp_Pln pln(ax2);
  const glm::vec3 origin = to_glm(pln.Location());
  const glm::vec3 normal = to_glm(pln.Axis().Direction());

  if (ray_pick::instance().pick_on_plane(xpos, ypos, disp, origin, normal,
                                         out_point)) {
    return true;
  }

  const auto ray =
      ray_pick::instance().screen_to_world_ray(xpos, ypos, disp);
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

profile_pick make_sketch_pick(handle sketch_h, const sketch_feature &sketch,
                              const glm::vec3 &anchor,
                              handle face_h = {}) {
  profile_pick pick;
  pick.source = profile_source_kind::sketch;
  pick.profile_face = sketch.profile_face();
  pick.direction = to_glm(sketch.plane().Direction());
  pick.anchor = anchor;
  pick.sketch_handle = sketch_h;
  pick.face_handle = face_h;
  return pick;
}

profile_pick make_face_pick(handle face_h, const brep_face &face_obj,
                            const glm::vec3 &anchor) {
  profile_pick pick;
  pick.source = profile_source_kind::face;
  pick.profile_face = face_obj.face();
  pick.direction = geometry::face_outward_normal(pick.profile_face);
  pick.anchor = anchor;
  pick.face_handle = face_h;
  return pick;
}

std::optional<profile_pick> pick_profile_from_ray(const glm::vec3 &ray_origin,
                                                  const glm::vec3 &ray_dir) {
  auto &db = instance::get().db();
  const auto hit = ray_cast_face(ray_origin, ray_dir);
  if (!hit.has_value()) {
    return std::nullopt;
  }

  const handle face_h = hit->first;
  const brep_face *face_obj = try_get_face(db, face_h);
  if (!face_obj) {
    return std::nullopt;
  }

  if (const sketch_feature *sketch =
          find_sketch_for_brep(db, face_obj->owner_brep())) {
    for (const handle feature_h : instance::get().history()) {
      if (db.try_get_as<sketch_feature>(feature_h) == sketch) {
        return make_sketch_pick(feature_h, *sketch, hit->second, face_h);
      }
    }
  }

  return make_face_pick(face_h, *face_obj, hit->second);
}

std::optional<profile_pick> pick_sketch_by_curves(double xpos, double ypos,
                                                  const disp_var &disp) {
  auto &db = instance::get().db();
  const glm::vec2 cursor(static_cast<float>(xpos),
                         static_cast<float>(ypos));

  const auto pick = pick_closest_sketch_curve(
      cursor, disp,
      [](handle, const sketch_feature &sketch) { return sketch.has_profile(); },
      k_sketch_curve_pick_px);
  if (!pick.has_value()) {
    return std::nullopt;
  }

  const auto *sketch =
      db.try_get_as<sketch_feature>(pick->curve.sketch);
  if (!sketch) {
    return std::nullopt;
  }

  glm::vec3 anchor{};
  if (!pick_on_sketch_plane(xpos, ypos, disp, sketch->plane(), anchor)) {
    return std::nullopt;
  }

  return make_sketch_pick(pick->curve.sketch, *sketch, anchor);
}

} // namespace

bool profile_input_handler::on_mouse_moved(double xpos, double ypos) {
  update_hover(xpos, ypos);

  if (owner()) {
    owner()->on_input_changed(flag(), this, false);
  }

  return true;
}

bool profile_input_handler::on_mouse_button(int button, int action, int mods) {
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

void profile_input_handler::update_hover(double xpos, double ypos) {
  auto &disp = instance::get().disp();
  const auto ray = ray_pick::instance().screen_to_world_ray(xpos, ypos, disp);

  m_hover_pick = {};

  if (auto pick = pick_profile_from_ray(ray.origin, ray.direction)) {
    m_hover_pick = *pick;
    if (m_hover_pick.face_handle.valid()) {
      selection_manager::instance().set_hover_target(
          subshape_ref{m_hover_pick.face_handle});
    } else {
      selection_manager::instance().clear_hover_target();
    }
    return;
  }

  if (auto pick = pick_sketch_by_curves(xpos, ypos, disp)) {
    m_hover_pick = *pick;
    if (m_hover_pick.face_handle.valid()) {
      selection_manager::instance().set_hover_target(
          subshape_ref{m_hover_pick.face_handle});
    } else {
      selection_manager::instance().clear_hover_target();
    }
    return;
  }

  selection_manager::instance().clear_hover_target();
}

} // namespace toy_cad::interaction
