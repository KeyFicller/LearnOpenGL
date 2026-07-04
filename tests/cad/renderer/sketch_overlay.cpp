#include "sketch_overlay.h"

#include "tests/cad/history/sketch_feature.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/sketch_context.h"
#include "tests/cad/interaction/sketch_plane_input_handler.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <cmath>
#include <optional>
#include <vector>

namespace toy_cad::renderer {

namespace {

constexpr int k_circle_segments = 64;
constexpr double k_datum_patch_half = 8.0;
constexpr float k_line_thickness = 2.5f;

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
                        const gp_Pnt &center, double radius, ImU32 color) {
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
    draw_list->AddLine(pts[i - 1], pts[i], color, k_line_thickness);
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

void draw_datum_plane_highlight(ImDrawList *draw_list, const gp_Ax2 &ax2) {
  const gp_Pnt c0 = plane_corner(ax2, -k_datum_patch_half, -k_datum_patch_half);
  const gp_Pnt c1 = plane_corner(ax2, k_datum_patch_half, -k_datum_patch_half);
  const gp_Pnt c2 = plane_corner(ax2, k_datum_patch_half, k_datum_patch_half);
  const gp_Pnt c3 = plane_corner(ax2, -k_datum_patch_half, k_datum_patch_half);

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

void draw_committed_sketch(const sketch_feature &sketch, ImDrawList *draw_list) {
  const ImU32 curve_color = IM_COL32(25, 215, 240, 255);
  for (const sketch_feature::line_segment &line : sketch.lines()) {
    draw_screen_line(draw_list, to_glm(line.p1), to_glm(line.p2), curve_color,
                     k_line_thickness);
  }
  for (const sketch_feature::circle_curve &circle : sketch.circles()) {
    draw_screen_circle(draw_list, sketch.plane(), circle.center, circle.radius,
                       curve_color);
  }
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
                         preview.circle_radius, preview_color);
    }
  }
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
    }
  }

  if (!ctx.is_active()) {
    return;
  }

  if (const sketch_feature *sketch = ctx.sketch()) {
    draw_committed_sketch(*sketch, draw_list);
  }

  draw_live_preview(ctx.live_preview(), draw_list);
}

} // namespace toy_cad::renderer
