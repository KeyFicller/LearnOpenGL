#include "box_feature.h"

#include "tests/cad/geometry/occt_mesh_utils.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/doc_input_handler.h"
#include "tests/cad/document/inspector_panel.h"
#include "tests/cad/interaction/inspector_tree_node.h"
#include "basic/shader.h"

#include "imgui.h"
#include "glad/gl.h"

#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Tool.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pln.hxx>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace toy_cad {

void box_feature::set_corners(const glm::vec3 &p1, const glm::vec3 &p2) {
  m_min.x = std::min(p1.x, p2.x);
  m_min.y = std::min(p1.y, p2.y);
  m_min.z = std::min(p1.z, p2.z);
  m_max.x = std::max(p1.x, p2.x);
  m_max.y = std::max(p1.y, p2.y);
  m_max.z = std::max(p1.z, p2.z);
  m_mesh.dirty = true;
}

void box_feature::regen() {
  // Create OCCT box from min/max corners
  gp_Pnt p1(m_min.x, m_min.y, m_min.z);
  gp_Pnt p2(m_max.x, m_max.y, m_max.z);

  // OCCT MakeBox creates a box from two corners
  // Note: BRepPrimAPI_MakeBox doesn't use IsDone() pattern like other
  // algorithms; it throws exceptions or produces degenerate shapes on error.
  // We check for non-zero dimensions instead.
  const double dx = std::abs(p2.X() - p1.X());
  const double dy = std::abs(p2.Y() - p1.Y());
  const double dz = std::abs(p2.Z() - p1.Z());
  constexpr double min_dim = 1e-6;

  // Debug output
  std::printf("[box_feature::regen] p1(%.4f,%.4f,%.4f) p2(%.4f,%.4f,%.4f) dims(%.4f,%.4f,%.4f)\n",
              p1.X(), p1.Y(), p1.Z(), p2.X(), p2.Y(), p2.Z(), dx, dy, dz);

  if (dx < min_dim || dy < min_dim || dz < min_dim) {
    // Degenerate box - dimensions too small
    std::printf("[box_feature::regen] Degenerate box rejected (dims too small)\n");
    m_shape_ok = false;
    m_mesh.dirty = true;
    m_faces.clear();
    m_edges.clear();
    m_bvh.clear();
    return;
  }

  try {
    BRepPrimAPI_MakeBox box_maker(p1, p2);
    m_shape = box_maker.Shape();
    m_shape_ok = !m_shape.IsNull();

    std::printf("[box_feature::regen] Box created, shape null=%s\n",
                m_shape.IsNull() ? "yes" : "no");

    if (m_shape_ok) {
      // Mesh the shape for rendering (linear deflection = 0.5)
      BRepMesh_IncrementalMesh mesh(m_shape, 0.5);
      std::printf("[box_feature::regen] Mesh generated\n");

      // Build subshape data and BVH for selection
      build_subshape_data();
    }
  } catch (...) {
    std::printf("[box_feature::regen] Exception caught during box creation\n");
    m_shape_ok = false;
    m_faces.clear();
    m_edges.clear();
    m_bvh.clear();
  }

  m_mesh.dirty = true;
}

void box_feature::build_subshape_data() {
  // Clear existing data
  m_faces.clear();
  m_edges.clear();

  if (m_shape.IsNull()) {
    m_bvh.clear();
    return;
  }

  // Extract faces and edges using geometry utilities
  geometry::extract_subshapes(m_shape, m_faces, m_edges);

  std::printf("[box_feature::build_subshape_data] Extracted %zu faces, %zu edges\n",
              m_faces.size(), m_edges.size());

  // Build BVH tree for fast intersection queries
  m_bvh.build(m_faces, m_edges);

  std::printf("[box_feature::build_subshape_data] BVH built%s\n",
              m_bvh.empty() ? " (empty)" : "");
}

void box_feature::draw_global() {
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

void box_feature::draw_local() {
  // Labels or screen-space effects would go here
}

void box_feature::draw_ui(handle explorer_row) {
  auto &doc = interaction::doc_input_handler::instance();

  std::string leaf = tag();
  if (leaf.empty()) {
    leaf = "Box";
  }
  leaf += "###box_" + std::to_string(explorer_row.index);

  [[maybe_unused]] const auto leaf_out =
      interaction::tree_leaf(leaf.c_str(), 0, explorer_row, doc);

  interaction::tree_item_context_menu menu;
  if (menu) {
    interaction::append_tree_inspector_menu_items(explorer_row);
    ImGui::Separator();
    ImGui::TextUnformatted(tag().empty() ? "Box" : tag().c_str());
    ImGui::Spacing();
    ImGui::Text("Min  (%.3f, %.3f, %.3f)", m_min.x, m_min.y, m_min.z);
    ImGui::Text("Max  (%.3f, %.3f, %.3f)", m_max.x, m_max.y, m_max.z);
    const float dx = m_max.x - m_min.x;
    const float dy = m_max.y - m_min.y;
    const float dz = m_max.z - m_min.z;
    ImGui::Text("Size (%.3f, %.3f, %.3f)", dx, dy, dz);
    ImGui::Text("Geometry: %s", m_shape_ok ? "valid" : "invalid");
  }
}

void box_feature::inspect() const {
  primitive_feature::inspect();
  ImGui::SeparatorText("box_feature");
  ImGui::BulletText("Min: (%.3f, %.3f, %.3f)", m_min.x, m_min.y, m_min.z);
  ImGui::BulletText("Max: (%.3f, %.3f, %.3f)", m_max.x, m_max.y, m_max.z);
  ImGui::BulletText("Geometry: %s", m_shape_ok ? "valid" : "invalid");
  ImGui::BulletText("Faces: %zu", m_faces.size());
  ImGui::BulletText("Edges: %zu", m_edges.size());
  ImGui::BulletText("BVH: %s", m_bvh.empty() ? "empty" : "built");
}

void box_feature::triangulate_shape() const {
  m_mesh.vertices.clear();
  m_mesh.normals.clear();
  m_mesh.indices.clear();

  if (m_shape.IsNull()) {
    return;
  }

  // Explore faces and triangulate
  for (TopExp_Explorer face_exp(m_shape, TopAbs_FACE); face_exp.More(); face_exp.Next()) {
    TopoDS_Face face = TopoDS::Face(face_exp.Current());

    // Get triangulation from face
    TopLoc_Location loc;
    Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);

    if (tri.IsNull()) {
      continue;
    }

    // Get transformation matrix
    gp_Trsf trsf = loc.Transformation();

    // Outward normal from BRep surface (matches OCCT face orientation)
    const glm::vec3 face_normal = geometry::face_outward_normal(face);

    // Base vertex index for this face
    unsigned int base_idx = static_cast<unsigned int>(m_mesh.vertices.size() / 3);

    // Add vertices and normals
    int n_nodes = tri->NbNodes();
    for (int i = 1; i <= n_nodes; ++i) {
      gp_Pnt p = tri->Node(i);
      p.Transform(trsf);
      m_mesh.vertices.push_back(static_cast<float>(p.X()));
      m_mesh.vertices.push_back(static_cast<float>(p.Y()));
      m_mesh.vertices.push_back(static_cast<float>(p.Z()));
      m_mesh.normals.push_back(face_normal.x);
      m_mesh.normals.push_back(face_normal.y);
      m_mesh.normals.push_back(face_normal.z);
    }

    // Triangle indices: align winding to outward normal (OpenGL CCW front face)
    int n_triangles = tri->NbTriangles();
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
      geometry::align_triangle_winding_to_outward(
          to_glm(n1), to_glm(n2), to_glm(n3), n2, n3, face_normal);

      m_mesh.indices.push_back(base_idx + static_cast<unsigned int>(n1 - 1));
      m_mesh.indices.push_back(base_idx + static_cast<unsigned int>(n2 - 1));
      m_mesh.indices.push_back(base_idx + static_cast<unsigned int>(n3 - 1));
    }
  }
}

void box_feature::upload_mesh() const {
  // Delete old buffers if they exist
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

  // Create and upload VAO/VBO/NBO/IBO
  glGenVertexArrays(1, &m_mesh.vao);
  glGenBuffers(1, &m_mesh.vbo);
  glGenBuffers(1, &m_mesh.nbo);
  glGenBuffers(1, &m_mesh.ibo);

  glBindVertexArray(m_mesh.vao);

  // Upload vertices (location 0)
  glBindBuffer(GL_ARRAY_BUFFER, m_mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER,
               m_mesh.vertices.size() * sizeof(float),
               m_mesh.vertices.data(),
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

  // Upload normals (location 1)
  glBindBuffer(GL_ARRAY_BUFFER, m_mesh.nbo);
  glBufferData(GL_ARRAY_BUFFER,
               m_mesh.normals.size() * sizeof(float),
               m_mesh.normals.data(),
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

  // Upload indices
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mesh.ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               m_mesh.indices.size() * sizeof(unsigned int),
               m_mesh.indices.data(),
               GL_STATIC_DRAW);

  glBindVertexArray(0);
}

void box_feature::draw_mesh() const {
  if (m_mesh.vao == 0 || m_mesh.indices.empty()) {
    return;
  }

  shader &sh = instance::get().global_shader();

  // Calculate model matrix (identity for now, box is already in world space)
  glm::mat4 model(1.0f);

  // Light settings
  glm::vec3 lightDir(0.5f, -1.0f, -0.5f); // Light coming from upper-right-front
  glm::vec3 ambientColor(0.3f, 0.3f, 0.35f);
  glm::vec3 diffuseColor(0.8f, 0.8f, 0.75f);
  glm::vec3 baseColor(0.6f, 0.65f, 0.7f); // Light gray-blue box color

  sh.use();
  sh.set_uniform("uMVP", instance::get().disp().clip_from_world());
  sh.set_uniform("uModel", model);
  sh.set_uniform("uCameraPos", instance::get().disp().camera_world_position);
  sh.set_uniform("uLightDir", lightDir);
  sh.set_uniform("uAmbientColor", ambientColor);
  sh.set_uniform("uDiffuseColor", diffuseColor);
  sh.set_uniform("uBaseColor", baseColor);

  glBindVertexArray(m_mesh.vao);
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_mesh.indices.size()), GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
}

std::optional<geometry::ray_hit> box_feature::intersect_ray(
    const glm::vec3 &ray_origin, const glm::vec3 &ray_dir) const {
  if (!has_bvh()) {
    return std::nullopt;
  }
  return m_bvh.intersect_ray(ray_origin, ray_dir);
}

void box_feature::intersect_frustum(
    const glm::vec4 frustum_planes[6],
    std::vector<const geometry::subshape_handle *> &out_hits) const {
  if (!has_bvh()) {
    out_hits.clear();
    return;
  }
  m_bvh.intersect_frustum(frustum_planes, out_hits);
}

void box_feature::draw_single_face(const geometry::subshape_handle &face_handle,
                                   const glm::vec3 &color) const {
  if (!face_handle.is_face()) return;

  const geometry::face_geometry &geom = face_handle.face_geom();
  if (geom.vertices.empty() || geom.indices.empty()) return;

  // Create temporary VAO for this face
  GLuint vao = 0, vbo = 0, nbo = 0, ibo = 0;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &nbo);
  glGenBuffers(1, &ibo);

  glBindVertexArray(vao);

  // Upload vertices (location 0)
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER,
               geom.vertices.size() * sizeof(glm::vec3),
               geom.vertices.data(),
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

  // Upload normals (location 1) - required by shader
  glBindBuffer(GL_ARRAY_BUFFER, nbo);
  glBufferData(GL_ARRAY_BUFFER,
               geom.normals.size() * sizeof(glm::vec3),
               geom.normals.data(),
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

  // Upload indices
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               geom.indices.size() * sizeof(uint32_t),
               geom.indices.data(),
               GL_STATIC_DRAW);

  // Use normal lighting with highlight color as base color
  // This ensures the highlighted face has proper shading like the original
  shader &sh = instance::get().global_shader();
  sh.use();
  sh.set_uniform("uMVP", instance::get().disp().clip_from_world());
  sh.set_uniform("uModel", glm::mat4(1.0f));
  sh.set_uniform("uCameraPos", instance::get().disp().camera_world_position);
  sh.set_uniform("uLightDir", glm::vec3(0.5f, -1.0f, -0.5f));
  sh.set_uniform("uAmbientColor", glm::vec3(0.3f, 0.3f, 0.35f));
  sh.set_uniform("uDiffuseColor", glm::vec3(0.8f, 0.8f, 0.75f));
  sh.set_uniform("uBaseColor", color);

  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(geom.indices.size()), GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);

  // Cleanup
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &nbo);
  glDeleteBuffers(1, &ibo);
}

void box_feature::draw_single_edge(const geometry::subshape_handle &edge_handle,
                                   const glm::vec3 &color) const {
  if (!edge_handle.is_edge()) return;

  const geometry::edge_geometry &geom = edge_handle.edge_geom();
  if (geom.vertices.size() < 2) return;

  // Create temporary VAO for this edge
  GLuint vao = 0, vbo = 0;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);

  // Upload vertices
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER,
               geom.vertices.size() * sizeof(glm::vec3),
               geom.vertices.data(),
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

  // Use viewport shader for unlit lines
  shader &sh = instance::get().viewport_shader();
  sh.use();
  sh.set_uniform("uMVP", instance::get().disp().clip_from_world());
  sh.set_uniform("uColor", glm::vec4(color, 1.0f));

  glBindVertexArray(vao);
  glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(geom.vertices.size()));
  glBindVertexArray(0);

  // Cleanup
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

void box_feature::draw_subshape_highlighted(const geometry::subshape_handle &handle,
                                           const glm::vec3 &color) const {
  if (handle.is_face()) {
    draw_single_face(handle, color);
  } else if (handle.is_edge()) {
    draw_single_edge(handle, color);
  }
}

void box_feature::draw_subshapes_highlighted(
    const std::vector<const geometry::subshape_handle *> &handles,
    const glm::vec3 &color) const {
  for (const auto *handle : handles) {
    if (handle) {
      draw_subshape_highlighted(*handle, color);
    }
  }
}

} // namespace toy_cad
