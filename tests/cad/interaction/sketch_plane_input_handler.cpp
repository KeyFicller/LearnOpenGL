#include "sketch_plane_input_handler.h"

#include "command_dispatcher.h"
#include "ray_pick.h"
#include "selection_manager.h"
#include "tests/cad/interaction/sketch_context.h"
#include "tests/cad/database/brep/brep.h"
#include "tests/cad/database/brep/brep_query.h"
#include "tests/cad/database/datum.h"
#include "tests/cad/history/coordinate.h"
#include "tests/cad/geometry/occt_mesh_utils.h"
#include "tests/cad/instance.h"

#include <BRepAdaptor_Surface.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>

#include "tests/framework/test_suit.h"
#include <GLFW/glfw3.h>
#include <limits>
#include <optional>

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

gp_Ax2 ax2_from_face(const TopoDS_Face &face) {
  BRepAdaptor_Surface surf(face);
  const gp_Ax3 ax3 = surf.Plane().Position();
  gp_Ax2 ax2(ax3.Location(), ax3.Direction(), ax3.XDirection());

  const glm::vec3 outward = geometry::face_outward_normal(face);
  gp_Dir normal(outward.x, outward.y, outward.z);
  ax2.SetDirection(normal);
  return ax2;
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

std::optional<sketch_plane_pick>
ray_cast_datum(const glm::vec3 &ray_origin, const glm::vec3 &ray_dir) {
  auto &db = instance::get().db();

  float closest_t = std::numeric_limits<float>::max();
  std::optional<sketch_plane_pick> closest;

  for (const handle feature_h : instance::get().history()) {
    auto *coord = db.try_get_as<coordinate>(feature_h);
    if (!coord) {
      continue;
    }

    for (const handle datum_h : coord->datum_handles()) {
      auto *datum_obj = db.try_get_as<datum>(datum_h);
      if (!datum_obj) {
        continue;
      }

      const gp_Pln pln = datum_obj->plane();
      const glm::vec3 plane_origin = to_glm(pln.Location());
      const glm::vec3 plane_normal = to_glm(pln.Axis().Direction());

      const float denom = glm::dot(ray_dir, plane_normal);
      if (std::abs(denom) < 1e-6f) {
        continue;
      }

      const float t = glm::dot(plane_origin - ray_origin, plane_normal) / denom;
      if (t <= 0.0f || t >= closest_t) {
        continue;
      }

      closest_t = t;
      sketch_plane_pick pick;
      pick.source = sketch_plane_source::datum;
      pick.ax2 = datum_obj->ax2();
      pick.source_handle = datum_h;
      closest = pick;
    }
  }

  return closest;
}

} // namespace

bool sketch_plane_input_handler::on_mouse_moved(double xpos, double ypos) {
  update_hover(xpos, ypos);

  if (owner()) {
    owner()->on_input_changed(flag(), this, false);
  }

  return true;
}

bool sketch_plane_input_handler::on_mouse_button(int button, int action,
                                                 int mods) {
  (void)mods;

  if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
    return false;
  }

  const glm::vec2 cursor = viewport_relative_cursor();
  update_hover(cursor.x, cursor.y);

  if (!has_valid_hover()) {
    return false;
  }

  if (owner()) {
    owner()->on_input_changed(flag(), this, true);
  }

  return true;
}

void sketch_plane_input_handler::update_hover(double xpos, double ypos) {
  auto &disp = instance::get().disp();
  const auto ray = ray_pick::instance().screen_to_world_ray(xpos, ypos, disp);

  m_hover_pick = {};

  const auto face_hit = ray_cast_face(ray.origin, ray.direction);
  if (face_hit.has_value()) {
    auto &db = instance::get().db();
    const brep_face *face_obj = try_get_face(db, face_hit->first);
    if (face_obj) {
      m_hover_pick.source = sketch_plane_source::brep_face;
      m_hover_pick.source_handle = face_hit->first;
      m_hover_pick.ax2 = ax2_from_face(face_obj->face());
      selection_manager::instance().set_hover_target(
          subshape_ref{face_hit->first});
      sketch_context::instance().set_plane_hover(m_hover_pick);
      return;
    }
  }

  const auto datum_hit = ray_cast_datum(ray.origin, ray.direction);
  if (datum_hit.has_value()) {
    m_hover_pick = *datum_hit;
    selection_manager::instance().clear_hover_target();
    sketch_context::instance().set_plane_hover(m_hover_pick);
    return;
  }

  selection_manager::instance().clear_hover_target();
  sketch_context::instance().set_plane_hover(std::nullopt);
}

} // namespace toy_cad::interaction
