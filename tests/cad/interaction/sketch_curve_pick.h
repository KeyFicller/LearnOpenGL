#pragma once

#include "tests/cad/database/handle.h"
#include "tests/cad/renderer/disp_var.h"

#include <glm/glm.hpp>
#include <cstddef>
#include <functional>
#include <optional>

namespace toy_cad {

class sketch_feature;

namespace interaction {

enum class sketch_curve_kind { line, circle };

/** Identifies one line segment or circle in a sketch feature. */
struct sketch_curve_ref {
  handle sketch{};
  sketch_curve_kind kind = sketch_curve_kind::line;
  std::size_t index = 0;

  friend bool operator==(const sketch_curve_ref &, const sketch_curve_ref &) =
      default;
};

struct sketch_curve_pick_result {
  sketch_curve_ref curve;
  float distance_px = 0.0f;
};

/** Screen-space distance from cursor to a world-space line segment. */
float screen_distance_to_segment(const glm::vec2 &cursor, const glm::vec3 &a_world,
                                 const glm::vec3 &b_world, const disp_var &disp);

/** Closest sketch curve within tolerance, if any. */
[[nodiscard]] std::optional<sketch_curve_pick_result>
pick_sketch_curve(const glm::vec2 &cursor, const disp_var &disp,
                  handle sketch_h, const sketch_feature &sketch,
                  float tolerance_px);

/** Closest curve across sketches that pass the include filter. */
[[nodiscard]] std::optional<sketch_curve_pick_result> pick_closest_sketch_curve(
    const glm::vec2 &cursor, const disp_var &disp,
    const std::function<bool(handle, const sketch_feature &)> &include,
    float tolerance_px);

} // namespace interaction
} // namespace toy_cad
