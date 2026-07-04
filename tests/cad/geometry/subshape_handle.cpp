#include "subshape_handle.h"

#include "occt_mesh_utils.h"

#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Surface.hxx>
#include <GeomLib_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <cmath>

namespace toy_cad::geometry {

float aabb::surface_area() const {
  const glm::vec3 e = extent();
  return 2.0f * (e.x * e.y + e.y * e.z + e.z * e.x);
}

glm::vec3 face_geometry::centroid() const {
  if (vertices.empty()) {
    return glm::vec3(0.0f);
  }

  glm::vec3 sum(0.0f);
  for (const auto &v : vertices) {
    sum += v;
  }
  return sum / static_cast<float>(vertices.size());
}

glm::vec3 edge_geometry::midpoint() const {
  if (vertices.empty()) {
    return glm::vec3(0.0f);
  }
  if (vertices.size() == 1) {
    return vertices[0];
  }
  return (vertices.front() + vertices.back()) * 0.5f;
}

subshape_handle subshape_handle::create_face(subshape_id id, const TopoDS_Face &face) {
  subshape_handle handle;
  handle.m_id = id;
  handle.m_face = face;
  handle.extract_face_geometry(face);
  return handle;
}

subshape_handle subshape_handle::create_edge(subshape_id id, const TopoDS_Edge &edge) {
  subshape_handle handle;
  handle.m_id = id;
  handle.m_edge = edge;
  handle.extract_edge_geometry(edge);
  return handle;
}

void subshape_handle::extract_face_geometry(const TopoDS_Face &face) {
  TopLoc_Location loc;
  Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, loc);

  if (triangulation.IsNull()) {
    // No triangulation available
    m_face_geom.bounds = aabb{};
    return;
  }

  const gp_Trsf trsf = loc.Transformation();

  // Get triangle count and reserve space
  const int n_triangles = triangulation->NbTriangles();
  m_face_geom.indices.reserve(n_triangles * 3);

  // Extract vertices
  const int n_nodes = triangulation->NbNodes();
  m_face_geom.vertices.reserve(n_nodes);
  m_face_geom.normals.reserve(n_nodes);

  // Transform and store vertices
  for (int i = 1; i <= n_nodes; ++i) {
    gp_Pnt p = triangulation->Node(i);
    p.Transform(trsf);
    m_face_geom.vertices.emplace_back(static_cast<float>(p.X()),
                                      static_cast<float>(p.Y()),
                                      static_cast<float>(p.Z()));
  }

  const glm::vec3 face_normal = face_outward_normal(face);

  // Fill normals (flat shading - same normal for all vertices)
  for (int i = 0; i < n_nodes; ++i) {
    m_face_geom.normals.push_back(face_normal);
  }

  // Extract indices with winding aligned to outward normal
  unsigned int base_idx = 0;
  for (int i = 1; i <= n_triangles; ++i) {
    Poly_Triangle tri = triangulation->Triangle(i);
    int n1 = 0, n2 = 0, n3 = 0;
    tri.Get(n1, n2, n3);

    const glm::vec3 p1 = m_face_geom.vertices[static_cast<size_t>(n1 - 1)];
    const glm::vec3 p2 = m_face_geom.vertices[static_cast<size_t>(n2 - 1)];
    const glm::vec3 p3 = m_face_geom.vertices[static_cast<size_t>(n3 - 1)];
    align_triangle_winding_to_outward(p1, p2, p3, n2, n3, face_normal);

    m_face_geom.indices.push_back(base_idx + static_cast<uint32_t>(n1 - 1));
    m_face_geom.indices.push_back(base_idx + static_cast<uint32_t>(n2 - 1));
    m_face_geom.indices.push_back(base_idx + static_cast<uint32_t>(n3 - 1));
  }

  // Compute bounding box
  if (!m_face_geom.vertices.empty()) {
    m_face_geom.bounds.min = m_face_geom.vertices[0];
    m_face_geom.bounds.max = m_face_geom.vertices[0];
    for (const auto &v : m_face_geom.vertices) {
      m_face_geom.bounds.expand(v);
    }
  }
}

void subshape_handle::extract_edge_geometry(const TopoDS_Edge &edge) {
  BRepAdaptor_Curve curve_adaptor(edge);

  // Get curve parameters
  const double first_param = curve_adaptor.FirstParameter();
  const double last_param = curve_adaptor.LastParameter();

  // Use quasi-uniform deflection for polyline approximation
  GCPnts_QuasiUniformDeflection discretizer;
  discretizer.Initialize(curve_adaptor, 0.1, first_param, last_param);

  if (discretizer.IsDone() && discretizer.NbPoints() > 0) {
    const int n_points = discretizer.NbPoints();
    m_edge_geom.vertices.reserve(n_points);

    for (int i = 1; i <= n_points; ++i) {
      const gp_Pnt p = discretizer.Value(i);
      m_edge_geom.vertices.emplace_back(static_cast<float>(p.X()),
                                        static_cast<float>(p.Y()),
                                        static_cast<float>(p.Z()));
    }
  } else {
    // Fallback: sample at endpoints and midpoint
    m_edge_geom.vertices.reserve(2);
    gp_Pnt p1 = curve_adaptor.Value(first_param);
    gp_Pnt p2 = curve_adaptor.Value(last_param);
    m_edge_geom.vertices.emplace_back(static_cast<float>(p1.X()),
                                      static_cast<float>(p1.Y()),
                                      static_cast<float>(p1.Z()));
    m_edge_geom.vertices.emplace_back(static_cast<float>(p2.X()),
                                      static_cast<float>(p2.Y()),
                                      static_cast<float>(p2.Z()));
  }

  // Compute bounding box
  if (!m_edge_geom.vertices.empty()) {
    m_edge_geom.bounds.min = m_edge_geom.vertices[0];
    m_edge_geom.bounds.max = m_edge_geom.vertices[0];
    for (const auto &v : m_edge_geom.vertices) {
      m_edge_geom.bounds.expand(v);
    }
    // Add small padding for zero-thickness edges
    const glm::vec3 padding(0.001f);
    m_edge_geom.bounds.min -= padding;
    m_edge_geom.bounds.max += padding;
  }
}

/**
 * Möller-Trumbore ray-triangle intersection.
 * @return t (distance along ray) if hit, -1.0f if miss
 */
static float ray_triangle_intersect(const glm::vec3 &orig, const glm::vec3 &dir,
                                    const glm::vec3 &v0, const glm::vec3 &v1,
                                    const glm::vec3 &v2) {
  const float EPSILON = 1e-6f;

  const glm::vec3 edge1 = v1 - v0;
  const glm::vec3 edge2 = v2 - v0;
  const glm::vec3 h = glm::cross(dir, edge2);
  const float a = glm::dot(edge1, h);

  if (std::abs(a) < EPSILON) {
    return -1.0f; // Ray parallel to triangle
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

  if (t > EPSILON) {
    return t;
  }

  return -1.0f;
}

float subshape_handle::intersect_ray(const glm::vec3 &ray_origin,
                                     const glm::vec3 &ray_dir) const {
  if (!is_face() || m_face_geom.vertices.empty() ||
      m_face_geom.indices.empty()) {
    return -1.0f;
  }

  float min_t = -1.0f;

  // Test all triangles
  for (size_t i = 0; i < m_face_geom.indices.size(); i += 3) {
    const uint32_t i0 = m_face_geom.indices[i];
    const uint32_t i1 = m_face_geom.indices[i + 1];
    const uint32_t i2 = m_face_geom.indices[i + 2];

    const glm::vec3 &v0 = m_face_geom.vertices[i0];
    const glm::vec3 &v1 = m_face_geom.vertices[i1];
    const glm::vec3 &v2 = m_face_geom.vertices[i2];

    const float t = ray_triangle_intersect(ray_origin, ray_dir, v0, v1, v2);

    if (t > 0.0f && (min_t < 0.0f || t < min_t)) {
      min_t = t;
    }
  }

  return min_t;
}

void extract_subshapes(const TopoDS_Shape &shape,
                       std::vector<subshape_handle> &out_faces,
                       std::vector<subshape_handle> &out_edges) {
  out_faces.clear();
  out_edges.clear();

  // Extract faces
  uint32_t face_idx = 0;
  for (TopExp_Explorer face_exp(shape, TopAbs_FACE); face_exp.More();
       face_exp.Next()) {
    TopoDS_Face face = TopoDS::Face(face_exp.Current());
    subshape_id id{subshape_type::face, face_idx++};
    out_faces.push_back(subshape_handle::create_face(id, face));
  }

  // Extract edges
  uint32_t edge_idx = 0;
  for (TopExp_Explorer edge_exp(shape, TopAbs_EDGE); edge_exp.More();
       edge_exp.Next()) {
    TopoDS_Edge edge = TopoDS::Edge(edge_exp.Current());
    subshape_id id{subshape_type::edge, edge_idx++};
    out_edges.push_back(subshape_handle::create_edge(id, edge));
  }
}

} // namespace toy_cad::geometry
