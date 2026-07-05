#include "extrude_feature.h"

#include "tests/cad/database/brep/brep.h"
#include "tests/cad/geometry/occt_mesh_utils.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/doc_input_handler.h"
#include "tests/cad/document/inspector_panel.h"
#include "tests/cad/interaction/inspector_tree_node.h"
#include "basic/shader.h"

#include "imgui.h"
#include "glad/gl.h"

#include <BRepPrimAPI_MakePrism.hxx>
#include <BRep_Tool.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace toy_cad {

void extrude_feature::set_profile(const TopoDS_Face &face,
                                  const glm::vec3 &direction,
                                  const glm::vec3 &anchor) {
  m_profile_face = face;
  m_has_profile = !face.IsNull();
  m_direction = direction;
  m_anchor = anchor;
  m_mesh.dirty = true;
}

void extrude_feature::set_depth(float depth) {
  if (std::abs(m_depth - depth) < 1e-6f) {
    return;
  }
  m_depth = depth;
  m_mesh.dirty = true;
}

void extrude_feature::regen() {
  if (!m_has_profile || m_profile_face.IsNull()) {
    m_shape_ok = false;
    m_mesh.dirty = true;
    clear_brep_topology();
    return;
  }

  constexpr double min_depth = 1e-6;
  if (std::abs(m_depth) < min_depth) {
    m_shape_ok = false;
    m_mesh.dirty = true;
    clear_brep_topology();
    return;
  }

  const glm::vec3 dir = glm::length(m_direction) > 1e-6f
                            ? glm::normalize(m_direction)
                            : glm::vec3(0.0f, 0.0f, 1.0f);
  const gp_Vec extrude_vec(dir.x * m_depth, dir.y * m_depth, dir.z * m_depth);

  try {
    BRepPrimAPI_MakePrism prism_maker(m_profile_face, extrude_vec);
    m_shape = prism_maker.Shape();
    m_shape_ok = !m_shape.IsNull();

    if (m_shape_ok) {
      BRepMesh_IncrementalMesh mesh(m_shape, 0.5);
      rebuild_brep_topology();
    } else {
      clear_brep_topology();
    }
  } catch (...) {
    std::printf("[extrude_feature::regen] Exception during extrusion\n");
    m_shape_ok = false;
    clear_brep_topology();
  }

  m_mesh.dirty = true;
}

void extrude_feature::clear_brep_topology() {
  auto &db = instance::get().db();
  if (m_brep.valid()) {
    if (auto *brep_obj = db.try_get_as<brep>(m_brep)) {
      brep_obj->clear(db);
    }
  }
}

void extrude_feature::rebuild_brep_topology() {
  auto &db = instance::get().db();
  if (!m_brep.valid()) {
    m_brep = db.emplace<brep>();
  }
  if (auto *brep_obj = db.try_get_as<brep>(m_brep)) {
    brep_obj->rebuild(db, m_brep, m_shape);
  }
}

void extrude_feature::draw_global() {
  if (!m_shape_ok) {
    return;
  }

  if (m_mesh.dirty) {
    triangulate_shape();
    upload_mesh();
    m_mesh.dirty = false;
  }

  if (!m_mesh.vertices.empty()) {
    draw_mesh();
  }
}

void extrude_feature::draw_local() {}

void extrude_feature::draw_ui(handle explorer_row) {
  auto &doc = interaction::doc_input_handler::instance();

  std::string leaf = tag();
  if (leaf.empty()) {
    leaf = "Extrude";
  }
  leaf += "###extrude_" + std::to_string(explorer_row.index);

  [[maybe_unused]] const auto leaf_out =
      interaction::tree_leaf(leaf.c_str(), 0, explorer_row, doc);

  interaction::tree_item_context_menu menu;
  if (menu) {
    interaction::append_tree_inspector_menu_items(explorer_row);
    ImGui::Separator();
    ImGui::TextUnformatted(tag().empty() ? "Extrude" : tag().c_str());
    ImGui::Spacing();
    ImGui::Text("Depth: %.3f", m_depth);
    ImGui::Text("Direction: (%.3f, %.3f, %.3f)", m_direction.x, m_direction.y,
                m_direction.z);
    ImGui::Text("Geometry: %s", m_shape_ok ? "valid" : "invalid");
  }
}

void extrude_feature::inspect() const {
  primitive_feature::inspect();
  ImGui::SeparatorText("extrude_feature");
  ImGui::BulletText("Depth: %.3f", m_depth);
  ImGui::BulletText("Direction: (%.3f, %.3f, %.3f)", m_direction.x,
                    m_direction.y, m_direction.z);
  ImGui::BulletText("Anchor: (%.3f, %.3f, %.3f)", m_anchor.x, m_anchor.y,
                    m_anchor.z);
  ImGui::BulletText("Geometry: %s", m_shape_ok ? "valid" : "invalid");
  if (m_brep.valid()) {
    auto &db = instance::get().db();
    if (auto *brep_obj = db.try_get_as<brep>(m_brep)) {
      ImGui::BulletText("Faces: %zu", brep_obj->face_count(db));
      ImGui::BulletText("Edges: %zu", brep_obj->edge_count(db));
    }
  }
}

void extrude_feature::triangulate_shape() const {
  m_mesh.vertices.clear();
  m_mesh.normals.clear();
  m_mesh.indices.clear();

  if (m_shape.IsNull()) {
    return;
  }

  for (TopExp_Explorer face_exp(m_shape, TopAbs_FACE); face_exp.More();
       face_exp.Next()) {
    TopoDS_Face face = TopoDS::Face(face_exp.Current());

    TopLoc_Location loc;
    Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
    if (tri.IsNull()) {
      continue;
    }

    gp_Trsf trsf = loc.Transformation();
    const unsigned int base_idx =
        static_cast<unsigned int>(m_mesh.vertices.size() / 3);

    const int n_nodes = tri->NbNodes();
    for (int i = 1; i <= n_nodes; ++i) {
      gp_Pnt p = tri->Node(i);
      p.Transform(trsf);
      m_mesh.vertices.push_back(static_cast<float>(p.X()));
      m_mesh.vertices.push_back(static_cast<float>(p.Y()));
      m_mesh.vertices.push_back(static_cast<float>(p.Z()));
      const glm::vec3 node_normal =
          geometry::mesh_node_outward_normal(face, tri, i);
      m_mesh.normals.push_back(node_normal.x);
      m_mesh.normals.push_back(node_normal.y);
      m_mesh.normals.push_back(node_normal.z);
    }

    const int n_triangles = tri->NbTriangles();
    for (int i = 1; i <= n_triangles; ++i) {
      Poly_Triangle triangle = tri->Triangle(i);
      int n1 = 0, n2 = 0, n3 = 0;
      triangle.Get(n1, n2, n3);

      auto to_glm = [&](int idx) {
        gp_Pnt p = tri->Node(idx);
        p.Transform(trsf);
        return glm::vec3(static_cast<float>(p.X()), static_cast<float>(p.Y()),
                         static_cast<float>(p.Z()));
      };
      const glm::vec3 winding_ref =
          geometry::mesh_triangle_winding_reference(face, tri, n1, n2, n3);
      geometry::align_triangle_winding_to_outward(
          to_glm(n1), to_glm(n2), to_glm(n3), n2, n3, winding_ref);

      m_mesh.indices.push_back(base_idx + static_cast<unsigned int>(n1 - 1));
      m_mesh.indices.push_back(base_idx + static_cast<unsigned int>(n2 - 1));
      m_mesh.indices.push_back(base_idx + static_cast<unsigned int>(n3 - 1));
    }
  }
}

void extrude_feature::upload_mesh() const {
  if (m_mesh.vao != 0) {
    glDeleteVertexArrays(1, &m_mesh.vao);
    glDeleteBuffers(1, &m_mesh.vbo);
    glDeleteBuffers(1, &m_mesh.nbo);
    glDeleteBuffers(1, &m_mesh.ibo);
  }

  if (m_mesh.vertices.empty() || m_mesh.indices.empty()) {
    m_mesh.vao = m_mesh.vbo = m_mesh.nbo = m_mesh.ibo = 0;
    return;
  }

  glGenVertexArrays(1, &m_mesh.vao);
  glGenBuffers(1, &m_mesh.vbo);
  glGenBuffers(1, &m_mesh.nbo);
  glGenBuffers(1, &m_mesh.ibo);

  glBindVertexArray(m_mesh.vao);

  glBindBuffer(GL_ARRAY_BUFFER, m_mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER, m_mesh.vertices.size() * sizeof(float),
               m_mesh.vertices.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

  glBindBuffer(GL_ARRAY_BUFFER, m_mesh.nbo);
  glBufferData(GL_ARRAY_BUFFER, m_mesh.normals.size() * sizeof(float),
               m_mesh.normals.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mesh.ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               m_mesh.indices.size() * sizeof(unsigned int),
               m_mesh.indices.data(), GL_STATIC_DRAW);

  glBindVertexArray(0);
}

void extrude_feature::draw_mesh() const {
  if (m_mesh.vao == 0 || m_mesh.indices.empty()) {
    return;
  }

  shader &sh = instance::get().global_shader();
  const glm::mat4 model(1.0f);

  sh.use();
  sh.set_uniform("uMVP", instance::get().disp().clip_from_world());
  sh.set_uniform("uModel", model);
  sh.set_uniform("uCameraPos", instance::get().disp().camera_world_position);
  sh.set_uniform("uLightDir", glm::vec3(0.5f, -1.0f, -0.5f));
  sh.set_uniform("uAmbientColor", glm::vec3(0.3f, 0.3f, 0.35f));
  sh.set_uniform("uDiffuseColor", glm::vec3(0.8f, 0.8f, 0.75f));
  sh.set_uniform("uBaseColor", glm::vec3(0.55f, 0.7f, 0.65f));

  glBindVertexArray(m_mesh.vao);
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_mesh.indices.size()),
                 GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
}

} // namespace toy_cad
