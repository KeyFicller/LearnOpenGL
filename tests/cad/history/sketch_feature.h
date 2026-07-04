#pragma once

#include "primitive_feature.h"
#include "tests/cad/database/handle.h"

#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>

#include <vector>

namespace toy_cad {

/** 2D sketch profile on a plane; builds a pickable BREP face for extrusion. */
class sketch_feature : public primitive_feature {
public:
  struct line_segment {
    gp_Pnt p1;
    gp_Pnt p2;
  };

  struct circle_curve {
    gp_Pnt center;
    double radius = 0.0;
  };

  sketch_feature() = default;

  void set_plane(const gp_Ax2 &plane);
  [[nodiscard]] const gp_Ax2 &plane() const { return m_plane; }

  void add_line(const gp_Pnt &p1, const gp_Pnt &p2);
  void add_circle(const gp_Pnt &center, double radius);

  [[nodiscard]] const std::vector<line_segment> &lines() const {
    return m_lines;
  }
  [[nodiscard]] const std::vector<circle_curve> &circles() const {
    return m_circles;
  }

  [[nodiscard]] bool has_profile() const { return m_has_profile; }
  [[nodiscard]] const TopoDS_Face &profile_face() const {
    return m_profile_face;
  }

  void regen() override;

  void draw_global() override;
  void draw_local() override;
  void draw_ui(handle explorer_row) override;
  void inspect() const override;

  [[nodiscard]] handle brep_handle() const override { return m_brep; }

  /** Draw a preview segment (world-space endpoints). */
  void draw_preview_segment(const gp_Pnt &p1, const gp_Pnt &p2) const;
  void draw_preview_circle(const gp_Pnt &center, double radius) const;
  void draw_preview_point(const gp_Pnt &point) const;

private:
  gp_Ax2 m_plane{};
  std::vector<line_segment> m_lines;
  std::vector<circle_curve> m_circles;

  TopoDS_Wire m_wire;
  TopoDS_Face m_profile_face;
  bool m_has_profile = false;
  handle m_brep{};

  void clear_brep_topology();
  void rebuild_brep_topology();
  void draw_curves() const;
};

} // namespace toy_cad
