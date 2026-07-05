#include "brep_face.h"

#include "tests/cad/geometry/occt_mesh_utils.h"
#include "tests/cad/instance.h"

#include "basic/shader.h"
#include "glad/gl.h"
#include "imgui.h"

#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>

#include <cmath>

namespace toy_cad {

namespace {

float ray_triangle_intersect(const glm::vec3 &orig, const glm::vec3 &dir,
                             const glm::vec3 &v0, const glm::vec3 &v1,
                             const glm::vec3 &v2) {
  constexpr float k_epsilon = 1e-6f;

  const glm::vec3 edge1 = v1 - v0;
  const glm::vec3 edge2 = v2 - v0;
  const glm::vec3 h = glm::cross(dir, edge2);
  const float a = glm::dot(edge1, h);

  if (std::abs(a) < k_epsilon) {
    return -1.0f;
  }

  const float f = 1.0f / a;
  const glm::vec3 s = orig - v0;
  const float u = f * glm::dot(s, h);

  if (u < 0.0f || u > 1.0f) {
    return -1.0f;
  }

  const glm::vec3 q = glm::cross(s, edge1);
  const float v = f * glm::dot(dir, q);

  if (v < 0.0f || u + v > 1.0f) {
    return -1.0f;
  }

  const float t = f * glm::dot(edge2, q);
  return t > k_epsilon ? t : -1.0f;
}

} // namespace

brep_face::brep_face(const TopoDS_Face &face, handle owner_brep,
                     handle owner_shell)
    : m_face(face), m_owner_brep(owner_brep), m_owner_shell(owner_shell) {
  build_geometry();
}

void brep_face::set_topology(const TopoDS_Face &face, handle owner_brep,
                             handle owner_shell) {
  m_face = face;
  m_owner_brep = owner_brep;
  m_owner_shell = owner_shell;
  build_geometry();
}

void brep_face::build_geometry() {
  m_geom.vertices.clear();
  m_geom.normals.clear();
  m_geom.indices.clear();
  m_geom.bounds = geometry::aabb{};

  TopLoc_Location loc;
  Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(m_face, loc);
  if (triangulation.IsNull()) {
    return;
  }

  const gp_Trsf trsf = loc.Transformation();
  const int n_triangles = triangulation->NbTriangles();
  m_geom.indices.reserve(static_cast<size_t>(n_triangles) * 3);

  const int n_nodes = triangulation->NbNodes();
  m_geom.vertices.reserve(static_cast<size_t>(n_nodes));
  m_geom.normals.reserve(static_cast<size_t>(n_nodes));

  for (int i = 1; i <= n_nodes; ++i) {
    gp_Pnt p = triangulation->Node(i);
    p.Transform(trsf);
    m_geom.vertices.emplace_back(static_cast<float>(p.X()),
                                 static_cast<float>(p.Y()),
                                 static_cast<float>(p.Z()));
  }

  for (int i = 1; i <= n_nodes; ++i) {
    const glm::vec3 node_normal =
        geometry::mesh_node_outward_normal(m_face, triangulation, i);
    m_geom.normals.push_back(node_normal);
  }

  const unsigned int base_idx = 0;
  for (int i = 1; i <= n_triangles; ++i) {
    Poly_Triangle tri = triangulation->Triangle(i);
    int n1 = 0, n2 = 0, n3 = 0;
    tri.Get(n1, n2, n3);

    const glm::vec3 p1 = m_geom.vertices[static_cast<size_t>(n1 - 1)];
    const glm::vec3 p2 = m_geom.vertices[static_cast<size_t>(n2 - 1)];
    const glm::vec3 p3 = m_geom.vertices[static_cast<size_t>(n3 - 1)];
    const glm::vec3 winding_ref = geometry::mesh_triangle_winding_reference(
        m_face, triangulation, n1, n2, n3);
    geometry::align_triangle_winding_to_outward(p1, p2, p3, n2, n3, winding_ref);

    m_geom.indices.push_back(base_idx + static_cast<uint32_t>(n1 - 1));
    m_geom.indices.push_back(base_idx + static_cast<uint32_t>(n2 - 1));
    m_geom.indices.push_back(base_idx + static_cast<uint32_t>(n3 - 1));
  }

  if (!m_geom.vertices.empty()) {
    m_geom.bounds.min = m_geom.vertices[0];
    m_geom.bounds.max = m_geom.vertices[0];
    for (const auto &v : m_geom.vertices) {
      m_geom.bounds.expand(v);
    }
  }
}

float brep_face::intersect_ray(const glm::vec3 &ray_origin,
                               const glm::vec3 &ray_dir) const {
  if (m_geom.vertices.empty() || m_geom.indices.empty()) {
    return -1.0f;
  }

  float min_t = -1.0f;
  for (size_t i = 0; i < m_geom.indices.size(); i += 3) {
    const uint32_t i0 = m_geom.indices[i];
    const uint32_t i1 = m_geom.indices[i + 1];
    const uint32_t i2 = m_geom.indices[i + 2];

    const float t = ray_triangle_intersect(
        ray_origin, ray_dir, m_geom.vertices[i0], m_geom.vertices[i1],
        m_geom.vertices[i2]);

    if (t > 0.0f && (min_t < 0.0f || t < min_t)) {
      min_t = t;
    }
  }
  return min_t;
}

void brep_face::draw_highlighted(const glm::vec3 &color) const {
  if (m_geom.vertices.empty() || m_geom.indices.empty()) {
    return;
  }

  GLuint vao = 0, vbo = 0, nbo = 0, ibo = 0;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &nbo);
  glGenBuffers(1, &ibo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, m_geom.vertices.size() * sizeof(glm::vec3),
               m_geom.vertices.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

  glBindBuffer(GL_ARRAY_BUFFER, nbo);
  glBufferData(GL_ARRAY_BUFFER, m_geom.normals.size() * sizeof(glm::vec3),
               m_geom.normals.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_geom.indices.size() * sizeof(uint32_t),
               m_geom.indices.data(), GL_STATIC_DRAW);

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
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_geom.indices.size()),
                 GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);

  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &nbo);
  glDeleteBuffers(1, &ibo);
}

void brep_face::inspect() const {
  object::inspect();
  ImGui::SeparatorText("brep_face");
  ImGui::BulletText("Vertices: %zu", m_geom.vertices.size());
  ImGui::BulletText("Triangles: %zu", m_geom.indices.size() / 3);
}

} // namespace toy_cad
