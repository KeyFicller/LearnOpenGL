#pragma once

#include "tests/cad/database/database.h"
#include "tests/cad/database/brep/brep.h"
#include "tests/cad/database/brep/brep_edge.h"
#include "tests/cad/database/brep/brep_face.h"
#include "tests/cad/geometry/subshape_handle.h"

#include <functional>
#include <optional>

namespace toy_cad {

class primitive_feature;

enum class brep_subshape_kind : uint8_t {
  unknown = 0,
  face = 1,
  edge = 2,
};

/** Resolve a db handle to face or edge subshape object. */
[[nodiscard]] brep_subshape_kind subshape_kind(database &db, handle h);

[[nodiscard]] brep_face *try_get_face(database &db, handle h);
[[nodiscard]] brep_edge *try_get_edge(database &db, handle h);

[[nodiscard]] const brep_face *try_get_face(const database &db, handle h);
[[nodiscard]] const brep_edge *try_get_edge(const database &db, handle h);

/** Bounds for box-select projection; nullopt if handle is stale or unknown. */
[[nodiscard]] std::optional<geometry::aabb> subshape_bounds(database &db,
                                                            handle h);

/** Draw highlight for a face or edge handle. */
void draw_subshape_highlighted(database &db, handle h, const glm::vec3 &color);

/** Iterate every brep owned by primitive features in the history tree. */
void for_each_scene_brep(const std::function<void(handle, brep &)> &fn);

/** Merged world-space bounds of all scene brep geometry; nullopt if empty. */
[[nodiscard]] std::optional<geometry::aabb> scene_bounds();

} // namespace toy_cad
