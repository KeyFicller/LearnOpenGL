#include "sketch_overlay.h"

#include "tests/cad/database/brep/brep_query.h"
#include "tests/cad/database/handle.h"
#include "tests/cad/geometry/subshape_handle.h"
#include "tests/cad/history/sketch_feature.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/doc_input_handler.h"
#include "tests/cad/interaction/selection_manager.h"
#include "tests/cad/interaction/sketch_context.h"
#include "tests/cad/interaction/sketch_curve_pick.h"
#include "tests/cad/interaction/sketch_plane_input_handler.h"
#include "tests/cad/renderer/cad_display_constants.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

namespace toy_cad::renderer {

namespace {

constexpr int k_circle_segments = 64;
constexpr float k_line_thickness = 2.5f;
constexpr float k_highlight_thickness = 5.0f;
constexpr float k_curve_pick_px = 10.0f;
constexpr double k_face_highlight_padding = 1.08;

constexpr ImU32 k_curve_color = IM_COL32(25, 215, 240, 255);
constexpr ImU32 k_hover_color = IM_COL32(255, 255, 0, 255);
constexpr ImU32 k_selected_color = IM_COL32(0, 255, 0, 255);

glm::vec3 to_glm(const gp_Pnt &p) {
  return glm::vec3(static_cast<float>(p.X()), static_cast<float>(p.Y()),
                   static_cast<float>(p.Z()));
}

std::optional<ImVec2> world_to_screen(const glm::vec3 &world) {
  ImGuiWindow *win = ImGui::FindWindowByName("Scene Viewport");
  if (!win || win->Hidden) {
    return std::nullopt;
  }

  const ImRect inner = win->InnerRect;
  if (inner.GetWidth() <= 1.0f || inner.GetHeight() <= 1.0f) {
    return std::nullopt;
  }

  const glm::mat4 mvp = instance::get().disp().clip_from_world();
  const glm::vec4 clip = mvp * glm::vec4(world, 1.0f);
  if (std::abs(clip.w) < 1e-6f) {
    return std::nullopt;
  }

  const float ndc_x = clip.x / clip.w;
  const float ndc_y = clip.y / clip.w;
  const float u = ndc_x * 0.5f + 0.5f;
  const float v = 1.0f - (ndc_y * 0.5f + 0.5f);
  return ImVec2(inner.Min.x + u * inner.GetWidth(),
                inner.Min.y + v * inner.GetHeight());
}

std::optional<glm::vec2> viewport_pick_cursor() {
  ImGuiWindow *win = ImGui::FindWindowByName("Scene Viewport");
  if (!win || win->Hidden) {
    return std::nullopt;
  }

  const ImRect inner = win->InnerRect;
  if (inner.GetWidth() <= 1.0f || inner.GetHeight() <= 1.0f) {
    return std::nullopt;
  }

  const ImVec2 mouse = ImGui::GetIO().MousePos;
  if (!inner.Contains(mouse)) {
    return std::nullopt;
  }

  const auto &disp = instance::get().disp();
  if (disp.render_width <= 0 || disp.render_height <= 0) {
    return std::nullopt;
  }

  const float u = (mouse.x - inner.Min.x) / inner.GetWidth();
  const float v = (mouse.y - inner.Min.y) / inner.GetHeight();
  return glm::vec2(u * static_cast<float>(disp.render_width),
                   v * static_cast<float>(disp.render_height));
}

void draw_screen_line(ImDrawList *draw_list, const glm::vec3 &a,
                      const glm::vec3 &b, ImU32 color, float thickness) {
  const auto sa = world_to_screen(a);
  const auto sb = world_to_screen(b);
  if (!sa.has_value() || !sb.has_value()) {
    return;
  }
  draw_list->AddLine(*sa, *sb, color, thickness);
}

void draw_screen_point_cross(ImDrawList *draw_list, const glm::vec3 &point,
                             ImU32 color, float half_px) {
  const auto sc = world_to_screen(point);
  if (!sc.has_value()) {
    return;
  }
  draw_list->AddLine(ImVec2(sc->x - half_px, sc->y),
                     ImVec2(sc->x + half_px, sc->y), color, k_line_thickness);
  draw_list->AddLine(ImVec2(sc->x, sc->y - half_px),
                     ImVec2(sc->x, sc->y + half_px), color, k_line_thickness);
}

void draw_screen_circle(ImDrawList *draw_list, const gp_Ax2 &plane,
                        const gp_Pnt &center, double radius, ImU32 color,
                        float thickness) {
  if (radius < 1e-6) {
    return;
  }

  const gp_Dir x_dir = plane.XDirection();
  const gp_Dir y_dir = gp_Dir(plane.Direction().XYZ().Crossed(x_dir.XYZ()));

  std::vector<ImVec2> pts;
  pts.reserve(k_circle_segments + 1);
  for (int i = 0; i <= k_circle_segments; ++i) {
    const double angle =
        (static_cast<double>(i) / static_cast<double>(k_circle_segments)) *
        2.0 * M_PI;
    gp_Pnt p = center;
    p.Translate(gp_Vec(x_dir) * (std::cos(angle) * radius));
    p.Translate(gp_Vec(y_dir) * (std::sin(angle) * radius));
    const auto sp = world_to_screen(to_glm(p));
    if (!sp.has_value()) {
      return;
    }
    pts.push_back(*sp);
  }

  for (std::size_t i = 1; i < pts.size(); ++i) {
    draw_list->AddLine(pts[i - 1], pts[i], color, thickness);
  }
}

gp_Pnt plane_corner(const gp_Ax2 &ax2, double sx, double sy) {
  gp_Pnt p = ax2.Location();
  p.Translate(gp_Vec(ax2.XDirection()) * sx);
  const gp_Dir y_dir =
      gp_Dir(ax2.Direction().XYZ().Crossed(ax2.XDirection().XYZ()));
  p.Translate(gp_Vec(y_dir) * sy);
  return p;
}

std::array<glm::vec3, 8> aabb_corners(const geometry::aabb &bounds) {
  return {{
      {bounds.min.x, bounds.min.y, bounds.min.z},
      {bounds.max.x, bounds.min.y, bounds.min.z},
      {bounds.max.x, bounds.max.y, bounds.min.z},
      {bounds.min.x, bounds.max.y, bounds.min.z},
      {bounds.min.x, bounds.min.y, bounds.max.z},
      {bounds.max.x, bounds.min.y, bounds.max.z},
      {bounds.max.x, bounds.max.y, bounds.max.z},
      {bounds.min.x, bounds.max.y, bounds.max.z},
  }};
}

void draw_plane_quad_highlight(ImDrawList *draw_list, const gp_Ax2 &ax2,
                               double u0, double v0, double u1, double v1) {
  const gp_Pnt c0 = plane_corner(ax2, u0, v0);
  const gp_Pnt c1 = plane_corner(ax2, u1, v0);
  const gp_Pnt c2 = plane_corner(ax2, u1, v1);
  const gp_Pnt c3 = plane_corner(ax2, u0, v1);

  const auto s0 = world_to_screen(to_glm(c0));
  const auto s1 = world_to_screen(to_glm(c1));
  const auto s2 = world_to_screen(to_glm(c2));
  const auto s3 = world_to_screen(to_glm(c3));
  if (!s0.has_value() || !s1.has_value() || !s2.has_value() ||
      !s3.has_value()) {
    return;
  }

  const ImU32 fill_color = IM_COL32(255, 255, 0, 48);
  const ImU32 border_color = IM_COL32(255, 255, 0, 220);
  draw_list->AddTriangleFilled(*s0, *s1, *s2, fill_color);
  draw_list->AddTriangleFilled(*s0, *s2, *s3, fill_color);
  const ImVec2 border[4] = {*s0, *s1, *s2, *s3};
  draw_list->AddPolyline(border, 4, border_color, ImDrawFlags_Closed,
                         k_line_thickness);
}

void draw_datum_plane_highlight(ImDrawList *draw_list, const gp_Ax2 &ax2) {
  const double extent = static_cast<double>(k_infinite_datum_display_extent);
  draw_plane_quad_highlight(draw_list, ax2, 0.0, 0.0, extent, extent);
}

void draw_face_plane_highlight(ImDrawList *draw_list, const gp_Ax2 &ax2,
                               const geometry::aabb &bounds) {
  const gp_Pnt origin = ax2.Location();
  const gp_Vec x_axis(ax2.XDirection());
  const gp_Dir y_dir =
      gp_Dir(ax2.Direction().XYZ().Crossed(ax2.XDirection().XYZ()));
  const gp_Vec y_axis(y_dir);

  double min_u = std::numeric_limits<double>::max();
  double max_u = std::numeric_limits<double>::lowest();
  double min_v = std::numeric_limits<double>::max();
  double max_v = std::numeric_limits<double>::lowest();

  for (const glm::vec3 &corner : aabb_corners(bounds)) {
    const gp_Vec rel(origin, gp_Pnt(corner.x, corner.y, corner.z));
    const double u = rel.Dot(x_axis);
    const double v = rel.Dot(y_axis);
    min_u = std::min(min_u, u);
    max_u = std::max(max_u, u);
    min_v = std::min(min_v, v);
    max_v = std::max(max_v, v);
  }

  if (min_u > max_u || min_v > max_v) {
    return;
  }

  const double cu = (min_u + max_u) * 0.5;
  const double cv = (min_v + max_v) * 0.5;
  const double hu = (max_u - min_u) * 0.5 * k_face_highlight_padding;
  const double hv = (max_v - min_v) * 0.5 * k_face_highlight_padding;
  draw_plane_quad_highlight(draw_list, ax2, cu - hu, cv - hv, cu + hu, cv + hv);
}

const sketch_feature::line_segment *
line_at(const sketch_feature &sketch, std::size_t index) {
  if (index >= sketch.lines().size()) {
    return nullptr;
  }
  return &sketch.lines()[index];
}

const sketch_feature::circle_curve *
circle_at(const sketch_feature &sketch, std::size_t index) {
  if (index >= sketch.circles().size()) {
    return nullptr;
  }
  return &sketch.circles()[index];
}

bool curve_matches(const interaction::sketch_curve_ref &ref,
                   interaction::sketch_curve_kind kind, std::size_t index) {
  return ref.kind == kind && ref.index == index;
}

void draw_curve_highlight(ImDrawList *draw_list, const sketch_feature &sketch,
                          interaction::sketch_curve_kind kind, std::size_t index,
                          ImU32 color) {
  if (kind == interaction::sketch_curve_kind::line) {
    if (const auto *line = line_at(sketch, index)) {
      draw_screen_line(draw_list, to_glm(line->p1), to_glm(line->p2), color,
                       k_highlight_thickness);
    }
    return;
  }

  if (const auto *circle = circle_at(sketch, index)) {
    draw_screen_circle(draw_list, sketch.plane(), circle->center,
                       circle->radius, color, k_highlight_thickness);
  }
}

void draw_committed_sketch(const sketch_feature &sketch, handle sketch_h,
                           ImDrawList *draw_list,
                           const interaction::sketch_curve_ref *hover_curve,
                           bool sketch_whole_hover, bool sketch_selected) {
  for (std::size_t i = 0; i < sketch.lines().size(); ++i) {
    const auto &line = sketch.lines()[i];
    const bool is_hover =
        sketch_whole_hover ||
        (hover_curve && hover_curve->sketch == sketch_h &&
         curve_matches(*hover_curve, interaction::sketch_curve_kind::line, i));
    if (is_hover || sketch_selected) {
      continue;
    }
    draw_screen_line(draw_list, to_glm(line.p1), to_glm(line.p2), k_curve_color,
                     k_line_thickness);
  }

  for (std::size_t i = 0; i < sketch.circles().size(); ++i) {
    const auto &circle = sketch.circles()[i];
    const bool is_hover =
        sketch_whole_hover ||
        (hover_curve && hover_curve->sketch == sketch_h &&
         curve_matches(*hover_curve, interaction::sketch_curve_kind::circle, i));
    if (is_hover || sketch_selected) {
      continue;
    }
    draw_screen_circle(draw_list, sketch.plane(), circle.center, circle.radius,
                       k_curve_color, k_line_thickness);
  }

  if (sketch_selected) {
    for (std::size_t i = 0; i < sketch.lines().size(); ++i) {
      const bool is_hover =
          sketch_whole_hover ||
          (hover_curve && hover_curve->sketch == sketch_h &&
           curve_matches(*hover_curve, interaction::sketch_curve_kind::line, i));
      if (is_hover) {
        continue;
      }
      draw_curve_highlight(draw_list, sketch,
                           interaction::sketch_curve_kind::line, i,
                           k_selected_color);
    }
    for (std::size_t i = 0; i < sketch.circles().size(); ++i) {
      const bool is_hover =
          sketch_whole_hover ||
          (hover_curve && hover_curve->sketch == sketch_h &&
           curve_matches(*hover_curve, interaction::sketch_curve_kind::circle,
                         i));
      if (is_hover) {
        continue;
      }
      draw_curve_highlight(draw_list, sketch,
                           interaction::sketch_curve_kind::circle, i,
                           k_selected_color);
    }
  }

  if (sketch_whole_hover) {
    for (std::size_t i = 0; i < sketch.lines().size(); ++i) {
      draw_curve_highlight(draw_list, sketch,
                           interaction::sketch_curve_kind::line, i,
                           k_hover_color);
    }
    for (std::size_t i = 0; i < sketch.circles().size(); ++i) {
      draw_curve_highlight(draw_list, sketch,
                           interaction::sketch_curve_kind::circle, i,
                           k_hover_color);
    }
    return;
  }

  if (hover_curve && hover_curve->sketch == sketch_h) {
    draw_curve_highlight(draw_list, sketch, hover_curve->kind,
                         hover_curve->index, k_hover_color);
  }
}

void draw_sketch(handle sketch_h, const sketch_feature &sketch,
                 ImDrawList *draw_list,
                 const interaction::sketch_curve_ref *hover_curve,
                 handle whole_hover_sketch) {
  const bool selected =
      interaction::doc_input_handler::instance().is_selected(sketch_h);
  const bool whole_hover = whole_hover_sketch.valid() &&
                           whole_hover_sketch == sketch_h;
  draw_committed_sketch(sketch, sketch_h, draw_list, hover_curve, whole_hover,
                        selected);
}

void draw_live_preview(const sketch_live_preview &preview,
                       ImDrawList *draw_list) {
  const ImU32 preview_color = IM_COL32(255, 215, 50, 255);

  if (preview.show_cursor) {
    draw_screen_point_cross(draw_list, preview.cursor_world, preview_color,
                            8.0f);
  }

  if (preview.show_segment) {
    draw_screen_line(draw_list, to_glm(preview.seg_p1), to_glm(preview.seg_p2),
                     preview_color, k_line_thickness);
  }

  if (preview.show_circle && preview.circle_radius > 1e-6) {
    if (const sketch_feature *sketch = sketch_context::instance().sketch()) {
      draw_screen_circle(draw_list, sketch->plane(), preview.circle_center,
                         preview.circle_radius, preview_color,
                         k_line_thickness);
    }
  }
}

std::optional<handle> hovered_sketch_from_subshape() {
  const auto hover = interaction::selection_manager::instance().hover_target();
  if (!hover.has_value()) {
    return std::nullopt;
  }

  auto &db = instance::get().db();
  handle owner_brep{};
  if (const brep_face *face = try_get_face(db, hover->subshape)) {
    owner_brep = face->owner_brep();
  } else if (const brep_edge *edge = try_get_edge(db, hover->subshape)) {
    owner_brep = edge->owner_brep();
  } else {
    return std::nullopt;
  }

  for (const handle feature_h : instance::get().history()) {
    const auto *sketch = db.try_get_as<sketch_feature>(feature_h);
    if (sketch && sketch->brep_handle() == owner_brep) {
      return feature_h;
    }
  }

  return std::nullopt;
}

} // namespace

void draw_sketch_viewport_overlay() {
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  if (!draw_list) {
    return;
  }

  auto &ctx = sketch_context::instance();

  if (const auto &hover = ctx.plane_hover()) {
    if (hover->source == interaction::sketch_plane_source::datum) {
      draw_datum_plane_highlight(draw_list, hover->ax2);
    } else if (hover->source == interaction::sketch_plane_source::brep_face &&
               hover->source_handle.valid()) {
      if (const std::optional<geometry::aabb> bounds =
              subshape_bounds(instance::get().db(), hover->source_handle)) {
        draw_face_plane_highlight(draw_list, hover->ax2, *bounds);
      }
    }
  }

  std::optional<interaction::sketch_curve_ref> hover_curve;
  handle whole_hover_sketch{};
  if (auto sketch_h = hovered_sketch_from_subshape()) {
    whole_hover_sketch = *sketch_h;
  } else if (const auto cursor = viewport_pick_cursor()) {
    if (auto pick = interaction::pick_closest_sketch_curve(
            *cursor, instance::get().disp(), {}, k_curve_pick_px)) {
      hover_curve = pick->curve;
    }
  }

  auto &db = instance::get().db();
  const interaction::sketch_curve_ref *hover_ptr =
      hover_curve.has_value() ? &hover_curve.value() : nullptr;

  if (ctx.is_active()) {
    if (const sketch_feature *sketch = ctx.sketch()) {
      draw_sketch(ctx.sketch_handle(), *sketch, draw_list, hover_ptr,
                  whole_hover_sketch);
    }
    draw_live_preview(ctx.live_preview(), draw_list);
    return;
  }

  for (const handle feature_h : instance::get().history()) {
    if (const sketch_feature *sketch = db.try_get_as<sketch_feature>(feature_h)) {
      draw_sketch(feature_h, *sketch, draw_list, hover_ptr, whole_hover_sketch);
    }
  }
}

} // namespace toy_cad::renderer
