#include "box_feature.h"

#include "tests/cad/instance.h"
#include "tests/cad/renderer/viewport_datum.h"
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
    }
  } catch (...) {
    std::printf("[box_feature::regen] Exception caught during box creation\n");
    m_shape_ok = false;
  }

  m_mesh.dirty = true;
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
  (void)explorer_row;
}

void box_feature::inspect() const {
  primitive_feature::inspect();
  ImGui::SeparatorText("box_feature");
  ImGui::BulletText("Min: (%.3f, %.3f, %.3f)", m_min.x, m_min.y, m_min.z);
  ImGui::BulletText("Max: (%.3f, %.3f, %.3f)", m_max.x, m_max.y, m_max.z);
  ImGui::BulletText("Geometry: %s", m_shape_ok ? "valid" : "invalid");
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

    // Get face normal from the surface plane
    // For a box face (planar), we can compute normal from first 3 points of the triangulation
    float nx = 0.0f, ny = 0.0f, nz = 1.0f; // Default Z-up normal

    if (tri->NbTriangles() > 0) {
      Poly_Triangle first_tri = tri->Triangle(1);
      int n1 = 0, n2 = 0, n3 = 0;
      first_tri.Get(n1, n2, n3);

      gp_Pnt p1 = tri->Node(n1);
      gp_Pnt p2 = tri->Node(n2);
      gp_Pnt p3 = tri->Node(n3);

      // Transform points
      p1.Transform(trsf);
      p2.Transform(trsf);
      p3.Transform(trsf);

      // Compute face normal from two edges
      gp_Vec edge1(p1, p2);
      gp_Vec edge2(p1, p3);
      gp_Vec normal = edge1.Crossed(edge2);

      if (normal.Magnitude() > 1e-10) {
        normal.Normalize();
        nx = static_cast<float>(normal.X());
        ny = static_cast<float>(normal.Y());
        nz = static_cast<float>(normal.Z());
      }
    }

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
      // Use face normal for each vertex (flat shading for box faces)
      m_mesh.normals.push_back(nx);
      m_mesh.normals.push_back(ny);
      m_mesh.normals.push_back(nz);
    }

    // Add triangle indices
    int n_triangles = tri->NbTriangles();
    for (int i = 1; i <= n_triangles; ++i) {
      Poly_Triangle triangle = tri->Triangle(i);
      int n1 = 0, n2 = 0, n3 = 0;
      triangle.Get(n1, n2, n3);

      // OCCT indices are 1-based, subtract 1 and add base_idx
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

} // namespace toy_cad
