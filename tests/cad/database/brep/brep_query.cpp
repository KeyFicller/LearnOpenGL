#include "brep_query.h"

#include "tests/cad/history/history_tree.h"
#include "tests/cad/history/primitive_feature.h"
#include "tests/cad/instance.h"

namespace toy_cad {

brep_subshape_kind subshape_kind(database &db, handle h) {
  if (try_get_face(db, h)) {
    return brep_subshape_kind::face;
  }
  if (try_get_edge(db, h)) {
    return brep_subshape_kind::edge;
  }
  return brep_subshape_kind::unknown;
}

brep_face *try_get_face(database &db, handle h) {
  return db.try_get_as<brep_face>(h);
}

brep_edge *try_get_edge(database &db, handle h) {
  return db.try_get_as<brep_edge>(h);
}

const brep_face *try_get_face(const database &db, handle h) {
  return db.try_get_as<brep_face>(h);
}

const brep_edge *try_get_edge(const database &db, handle h) {
  return db.try_get_as<brep_edge>(h);
}

std::optional<geometry::aabb> subshape_bounds(database &db, handle h) {
  if (auto *face = try_get_face(db, h)) {
    return face->bounds();
  }
  if (auto *edge = try_get_edge(db, h)) {
    return edge->bounds();
  }
  return std::nullopt;
}

void draw_subshape_highlighted(database &db, handle h, const glm::vec3 &color) {
  if (auto *face = try_get_face(db, h)) {
    face->draw_highlighted(color);
    return;
  }
  if (auto *edge = try_get_edge(db, h)) {
    edge->draw_highlighted(color);
  }
}

void for_each_scene_brep(const std::function<void(handle, brep &)> &fn) {
  if (!fn) {
    return;
  }

  auto &db = instance::get().db();
  for (const handle feature_h : instance::get().history()) {
    auto *primitive = db.try_get_as<primitive_feature>(feature_h);
    if (!primitive || !primitive->has_brep()) {
      continue;
    }
    auto *brep_obj = db.try_get_as<brep>(primitive->brep_handle());
    if (brep_obj) {
      fn(primitive->brep_handle(), *brep_obj);
    }
  }
}

std::optional<geometry::aabb> scene_bounds() {
  auto &db = instance::get().db();
  std::optional<geometry::aabb> merged;

  for_each_scene_brep([&](handle /*brep_h*/, brep &brep_obj) {
    brep_obj.for_each_subshape_handle(db, [&](handle sub_h) {
      const std::optional<geometry::aabb> bounds = subshape_bounds(db, sub_h);
      if (!bounds.has_value()) {
        return;
      }
      if (!merged.has_value()) {
        merged = *bounds;
      } else {
        merged->expand(*bounds);
      }
    });
  });

  return merged;
}

} // namespace toy_cad
