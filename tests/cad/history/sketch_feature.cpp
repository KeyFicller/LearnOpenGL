#include "sketch_feature.h"

#include "tests/cad/database/brep/brep.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/doc_input_handler.h"
#include "tests/cad/document/inspector_panel.h"
#include "tests/cad/interaction/inspector_tree_node.h"
#include "basic/shader.h"

#include "imgui.h"
#include "glad/gl.h"

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace toy_cad {

namespace {

constexpr int k_circle_segments = 64;

std::vector<glm::vec3> segment_vertices(const gp_Pnt &p1, const gp_Pnt &p2) {
  return {glm::vec3(static_cast<float>(p1.X()), static_cast<float>(p1.Y()),
                   static_cast<float>(p1.Z())),
          glm::vec3(static_cast<float>(p2.X()), static_cast<float>(p2.Y()),
                   static_cast<float>(p2.Z()))};
}

std::vector<glm::vec3> circle_vertices(const gp_Ax2 &plane,
                                       const gp_Pnt &center, double radius) {
  std::vector<glm::vec3> verts;
  if (radius <= 1e-9) {
    return verts;
  }

  const gp_Dir normal = plane.Direction();
  const gp_Dir x_dir = plane.XDirection();
  const gp_Dir y_dir = gp_Dir(normal.XYZ().Crossed(x_dir.XYZ()));

  verts.reserve(k_circle_segments + 1);
  for (int i = 0; i <= k_circle_segments; ++i) {
    const double angle =
        (static_cast<double>(i) / static_cast<double>(k_circle_segments)) *
        2.0 * M_PI;
    const double cx = std::cos(angle) * radius;
    const double cy = std::sin(angle) * radius;
    gp_Pnt p = center;
    p.Translate(gp_Vec(x_dir) * cx);
    p.Translate(gp_Vec(y_dir) * cy);
    verts.emplace_back(static_cast<float>(p.X()), static_cast<float>(p.Y()),
                       static_cast<float>(p.Z()));
  }
  return verts;
}

void draw_polyline(const std::vector<glm::vec3> &points, const glm::vec4 &color) {
  if (points.size() < 2) {
    return;
  }

  GLboolean depth_was = glIsEnabled(GL_DEPTH_TEST);
  glDisable(GL_DEPTH_TEST);

  GLfloat prev_line_width = 1.0f;
  glGetFloatv(GL_LINE_WIDTH, &prev_line_width);
  glLineWidth(2.5f);

  shader &sh = instance::get().viewport_shader();
  sh.use();
  sh.set_uniform("uMVP", instance::get().disp().clip_from_world());
  sh.set_uniform("uColor", color);

  std::vector<float> flat;
  flat.reserve(points.size() * 3);
  for (const glm::vec3 &p : points) {
    flat.push_back(p.x);
    flat.push_back(p.y);
    flat.push_back(p.z);
  }

  unsigned int vao = 0;
  unsigned int vbo = 0;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, flat.size() * sizeof(float), flat.data(),
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(points.size()));
  glBindVertexArray(0);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);

  glLineWidth(prev_line_width);
  if (depth_was) {
    glEnable(GL_DEPTH_TEST);
  }
}

} // namespace

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

void sketch_feature::draw_curves() const {
  const glm::vec4 color(0.1f, 0.85f, 0.95f, 1.0f);
  for (const line_segment &line : m_lines) {
    draw_polyline(segment_vertices(line.p1, line.p2), color);
  }
  for (const circle_curve &circle : m_circles) {
    draw_polyline(circle_vertices(m_plane, circle.center, circle.radius),
                  color);
  }
}

void sketch_feature::draw_preview_segment(const gp_Pnt &p1,
                                          const gp_Pnt &p2) const {
  draw_polyline(segment_vertices(p1, p2),
                glm::vec4(1.0f, 0.85f, 0.2f, 1.0f));
}

void sketch_feature::draw_preview_circle(const gp_Pnt &center,
                                         double radius) const {
  draw_polyline(circle_vertices(m_plane, center, radius),
                glm::vec4(1.0f, 0.85f, 0.2f, 1.0f));
}

void sketch_feature::draw_preview_point(const gp_Pnt &point) const {
  const gp_Dir x_dir = m_plane.XDirection();
  const gp_Dir y_dir = gp_Dir(m_plane.Direction().XYZ().Crossed(x_dir.XYZ()));
  constexpr double k_half = 0.12;

  gp_Pnt px1 = point;
  gp_Pnt px2 = point;
  px1.Translate(gp_Vec(x_dir) * -k_half);
  px2.Translate(gp_Vec(x_dir) * k_half);

  gp_Pnt py1 = point;
  gp_Pnt py2 = point;
  py1.Translate(gp_Vec(y_dir) * -k_half);
  py2.Translate(gp_Vec(y_dir) * k_half);

  const glm::vec4 color(1.0f, 0.85f, 0.2f, 1.0f);
  draw_polyline(segment_vertices(px1, px2), color);
  draw_polyline(segment_vertices(py1, py2), color);
}

void sketch_feature::draw_global() { draw_curves(); }

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
