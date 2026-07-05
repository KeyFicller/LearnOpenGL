#include "sketch_feature.h"

#include "tests/cad/database/brep/brep.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/doc_input_handler.h"
#include "tests/cad/document/inspector_panel.h"
#include "tests/cad/interaction/inspector_tree_node.h"

#include "imgui.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Pln.hxx>

#include <string>

namespace toy_cad {

void sketch_feature::set_plane(const gp_Ax2 &plane) { m_plane = plane; }

void sketch_feature::add_line(const gp_Pnt &p1, const gp_Pnt &p2) {
  if (p1.Distance(p2) < 1e-6) {
    return;
  }
  m_lines.push_back({p1, p2});
}

void sketch_feature::add_circle(const gp_Pnt &center, double radius) {
  if (radius < 1e-6) {
    return;
  }
  m_circles.push_back({center, radius});
}

void sketch_feature::regen() {
  m_has_profile = false;
  m_profile_face = TopoDS_Face();
  m_wire = TopoDS_Wire();
  clear_brep_topology();

  BRepBuilderAPI_MakeWire wire_maker;
  bool has_edges = false;

  for (const line_segment &line : m_lines) {
    BRepBuilderAPI_MakeEdge edge_maker(line.p1, line.p2);
    if (edge_maker.IsDone()) {
      wire_maker.Add(edge_maker.Edge());
      has_edges = true;
    }
  }

  for (const circle_curve &circle : m_circles) {
    gp_Ax2 circle_ax(m_plane.Location(), m_plane.Direction(), m_plane.XDirection());
    circle_ax.SetLocation(circle.center);
    gp_Circ circ(circle_ax, circle.radius);
    BRepBuilderAPI_MakeEdge edge_maker(circ);
    if (edge_maker.IsDone()) {
      wire_maker.Add(edge_maker.Edge());
      has_edges = true;
    }
  }

  if (!has_edges || !wire_maker.IsDone()) {
    return;
  }

  m_wire = wire_maker.Wire();

  try {
    BRepBuilderAPI_MakeFace face_maker(gp_Pln(m_plane), m_wire);
    if (face_maker.IsDone()) {
      m_profile_face = face_maker.Face();
      m_has_profile = !m_profile_face.IsNull();
      if (m_has_profile) {
        rebuild_brep_topology();
      }
    }
  } catch (...) {
    m_has_profile = false;
  }
}

void sketch_feature::clear_brep_topology() {
  auto &db = instance::get().db();
  if (m_brep.valid()) {
    if (auto *brep_obj = db.try_get_as<brep>(m_brep)) {
      brep_obj->clear(db);
    }
  }
}

void sketch_feature::rebuild_brep_topology() {
  auto &db = instance::get().db();
  if (!m_brep.valid()) {
    m_brep = db.emplace<brep>();
  }
  if (auto *brep_obj = db.try_get_as<brep>(m_brep)) {
    brep_obj->rebuild(db, m_brep, m_profile_face);
  }
}

void sketch_feature::draw_global() {}

void sketch_feature::draw_local() {}

void sketch_feature::draw_ui(handle explorer_row) {
  auto &doc = interaction::doc_input_handler::instance();

  std::string leaf = tag();
  if (leaf.empty()) {
    leaf = "Sketch";
  }
  leaf += "###sketch_" + std::to_string(explorer_row.index);

  [[maybe_unused]] const auto leaf_out =
      interaction::tree_leaf(leaf.c_str(), 0, explorer_row, doc);

  interaction::tree_item_context_menu menu;
  if (menu) {
    interaction::append_tree_inspector_menu_items(explorer_row);
    ImGui::Separator();
    ImGui::TextUnformatted(tag().empty() ? "Sketch" : tag().c_str());
    ImGui::Spacing();
    ImGui::Text("Lines: %zu", m_lines.size());
    ImGui::Text("Circles: %zu", m_circles.size());
    ImGui::Text("Profile: %s", m_has_profile ? "closed" : "open");
  }
}

void sketch_feature::inspect() const {
  primitive_feature::inspect();
  ImGui::SeparatorText("sketch_feature");
  ImGui::BulletText("Lines: %zu", m_lines.size());
  ImGui::BulletText("Circles: %zu", m_circles.size());
  ImGui::BulletText("Profile face: %s", m_has_profile ? "valid" : "none");
}

} // namespace toy_cad
