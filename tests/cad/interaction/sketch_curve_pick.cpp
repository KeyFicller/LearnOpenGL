#include "sketch_curve_pick.h"

#include "sketch_context.h"
#include "tests/cad/history/sketch_feature.h"
#include "tests/cad/instance.h"

#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <cmath>
#include <limits>

namespace toy_cad::interaction {

namespace {

glm::vec3 to_glm(const gp_Pnt &p) {
  return glm::vec3(static_cast<float>(p.X()), static_cast<float>(p.Y()),
                   static_cast<float>(p.Z()));
}

float screen_distance_px(const glm::vec2 &a, const glm::vec2 &b) {
  return glm::length(a - b);
}

} // namespace

float screen_distance_to_segment(const glm::vec2 &cursor, const glm::vec3 &a_world,
                                 const glm::vec3 &b_world, const disp_var &disp) {
  const glm::mat4 mvp = disp.clip_from_world();
  const auto project = [&](const glm::vec3 &world) -> std::optional<glm::vec2> {
    const glm::vec4 clip = mvp * glm::vec4(world, 1.0f);
    if (std::abs(clip.w) < 1e-6f) {
      return std::nullopt;
    }
    const float ndc_x = clip.x / clip.w;
    const float ndc_y = clip.y / clip.w;
    const float u = ndc_x * 0.5f + 0.5f;
    const float v = 1.0f - (ndc_y * 0.5f + 0.5f);
    const float sx = u * static_cast<float>(disp.render_width);
    const float sy = v * static_cast<float>(disp.render_height);
    return glm::vec2(sx, sy);
  };

  const auto sa = project(a_world);
  const auto sb = project(b_world);
  if (!sa.has_value() || !sb.has_value()) {
    return std::numeric_limits<float>::max();
  }

  const glm::vec2 ab = *sb - *sa;
  const float len_sq = glm::dot(ab, ab);
  if (len_sq < 1e-8f) {
    return screen_distance_px(cursor, *sa);
  }
  const float t =
      glm::clamp(glm::dot(cursor - *sa, ab) / len_sq, 0.0f, 1.0f);
  const glm::vec2 closest = *sa + ab * t;
  return screen_distance_px(cursor, closest);
}

std::optional<sketch_curve_pick_result>
pick_sketch_curve(const glm::vec2 &cursor, const disp_var &disp,
                  handle sketch_h, const sketch_feature &sketch,
                  float tolerance_px) {
  if (!sketch_h.valid()) {
    return std::nullopt;
  }

  float best_dist = tolerance_px;
  std::optional<sketch_curve_pick_result> best;

  for (std::size_t i = 0; i < sketch.lines().size(); ++i) {
    const sketch_feature::line_segment &line = sketch.lines()[i];
    const float dist = screen_distance_to_segment(cursor, to_glm(line.p1),
                                                  to_glm(line.p2), disp);
    if (dist >= best_dist) {
      continue;
    }
    best_dist = dist;
    best = sketch_curve_pick_result{
        {sketch_h, sketch_curve_kind::line, i}, dist};
  }

  const gp_Ax2 &plane = sketch.plane();
  const gp_Dir x_dir = plane.XDirection();
  const gp_Dir y_dir =
      gp_Dir(plane.Direction().XYZ().Crossed(x_dir.XYZ()));

  for (std::size_t i = 0; i < sketch.circles().size(); ++i) {
    const sketch_feature::circle_curve &circle = sketch.circles()[i];
    if (circle.radius < 1e-6) {
      continue;
    }

    constexpr int segments = 32;
    gp_Pnt prev = circle.center;
    prev.Translate(gp_Vec(x_dir) * circle.radius);
    for (int seg = 1; seg <= segments; ++seg) {
      const double angle =
          (static_cast<double>(seg) / static_cast<double>(segments)) * 2.0 *
          M_PI;
      gp_Pnt p = circle.center;
      p.Translate(gp_Vec(x_dir) * (std::cos(angle) * circle.radius));
      p.Translate(gp_Vec(y_dir) * (std::sin(angle) * circle.radius));
      const float dist = screen_distance_to_segment(cursor, to_glm(prev),
                                                    to_glm(p), disp);
      if (dist < best_dist) {
        best_dist = dist;
        best = sketch_curve_pick_result{
            {sketch_h, sketch_curve_kind::circle, i}, dist};
      }
      prev = p;
    }
  }

  return best;
}

std::optional<sketch_curve_pick_result> pick_closest_sketch_curve(
    const glm::vec2 &cursor, const disp_var &disp,
    const std::function<bool(handle, const sketch_feature &)> &include,
    float tolerance_px) {
  auto &db = instance::get().db();
  float best_dist = tolerance_px;
  std::optional<sketch_curve_pick_result> best;

  const auto try_sketch = [&](handle feature_h) {
    const auto *sketch = db.try_get_as<sketch_feature>(feature_h);
    if (!sketch || (include && !include(feature_h, *sketch))) {
      return;
    }
    if (auto pick = pick_sketch_curve(cursor, disp, feature_h, *sketch, best_dist)) {
      if (pick->distance_px < best_dist) {
        best_dist = pick->distance_px;
        best = pick;
      }
    }
  };

  for (const handle feature_h : instance::get().history()) {
    try_sketch(feature_h);
  }

  if (sketch_context::instance().is_active()) {
    try_sketch(sketch_context::instance().sketch_handle());
  }

  return best;
}

} // namespace toy_cad::interaction
