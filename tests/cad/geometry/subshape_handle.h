#pragma once

#include <glm/glm.hpp>

#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>

#include <cstdint>
#include <vector>

namespace toy_cad::geometry {

/**
 * Type of geometric sub-shape.
 */
enum class subshape_type : uint8_t {
  face = 0,
  edge = 1,
};

/**
 * Unique identifier for a subshape within a feature.
 * Combines type and index for stable identification.
 */
struct subshape_id {
  subshape_type type;
  uint32_t index;

  [[nodiscard]] bool valid() const { return index != 0xffffffffu; }

  [[nodiscard]] uint64_t key() const {
    return (static_cast<uint64_t>(static_cast<uint8_t>(type)) << 32u) | index;
  }

  friend bool operator==(subshape_id a, subshape_id b) {
    return a.type == b.type && a.index == b.index;
  }
  friend bool operator!=(subshape_id a, subshape_id b) { return !(a == b); }
  friend bool operator<(subshape_id a, subshape_id b) {
    return a.key() < b.key();
  }
};

/**
 * Hash function for subshape_id.
 */
struct subshape_id_hash {
  [[nodiscard]] std::size_t operator()(subshape_id id) const noexcept {
    return static_cast<std::size_t>(id.key());
  }
};

/**
 * Invalid subshape ID constant.
 */
inline constexpr subshape_id invalid_subshape_id{subshape_type::face, 0xffffffffu};

/**
 * Axis-aligned bounding box in world space.
 */
struct aabb {
  glm::vec3 min{0.0f};
  glm::vec3 max{0.0f};

  [[nodiscard]] bool valid() const {
    return min.x <= max.x && min.y <= max.y && min.z <= max.z;
  }

  [[nodiscard]] glm::vec3 center() const { return (min + max) * 0.5f; }
  [[nodiscard]] glm::vec3 extent() const { return max - min; }
  [[nodiscard]] float surface_area() const;

  /**
   * Expand the AABB to include a point.
   */
  void expand(const glm::vec3 &p) {
    min = glm::min(min, p);
    max = glm::max(max, p);
  }

  /**
   * Expand the AABB to include another AABB.
   */
  void expand(const aabb &other) {
    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
  }

  /**
   * Check if this AABB intersects with another.
   */
  [[nodiscard]] bool intersects(const aabb &other) const {
    return min.x <= other.max.x && max.x >= other.min.x &&
           min.y <= other.max.y && max.y >= other.min.y &&
           min.z <= other.max.z && max.z >= other.min.z;
  }

  /**
   * Check if a point is inside this AABB.
   */
  [[nodiscard]] bool contains(const glm::vec3 &p) const {
    return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y &&
           p.z >= min.z && p.z <= max.z;
  }
};

/**
 * Geometric data for a face (triangulated mesh).
 */
struct face_geometry {
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  std::vector<uint32_t> indices;
  aabb bounds;

  /**
   * Compute face centroid from vertices.
   */
  [[nodiscard]] glm::vec3 centroid() const;
};

/**
 * Geometric data for an edge (polyline).
 */
struct edge_geometry {
  std::vector<glm::vec3> vertices;
  aabb bounds;

  /**
   * Compute edge midpoint.
   */
  [[nodiscard]] glm::vec3 midpoint() const;
};

/**
 * Handle for a subshape (face or edge) within a BRep feature.
 * Stores the OCCT topology reference and cached geometric data.
 */
class subshape_handle {
public:
  subshape_handle() = default;

  /**
   * Create a face handle.
   */
  static subshape_handle create_face(subshape_id id, const TopoDS_Face &face);

  /**
   * Create an edge handle.
   */
  static subshape_handle create_edge(subshape_id id, const TopoDS_Edge &edge);

  [[nodiscard]] subshape_id id() const { return m_id; }
  [[nodiscard]] subshape_type type() const { return m_id.type; }
  [[nodiscard]] bool valid() const { return m_id.valid(); }

  [[nodiscard]] bool is_face() const { return m_id.type == subshape_type::face; }
  [[nodiscard]] bool is_edge() const { return m_id.type == subshape_type::edge; }

  /**
   * Get the OCCT face (valid only if is_face() is true).
   */
  [[nodiscard]] const TopoDS_Face &face() const { return m_face; }

  /**
   * Get the OCCT edge (valid only if is_edge() is true).
   */
  [[nodiscard]] const TopoDS_Edge &edge() const { return m_edge; }

  /**
   * Get the cached geometry for faces.
   */
  [[nodiscard]] const face_geometry &face_geom() const { return m_face_geom; }

  /**
   * Get the cached geometry for edges.
   */
  [[nodiscard]] const edge_geometry &edge_geom() const { return m_edge_geom; }

  /**
   * Get the world-space bounding box.
   */
  [[nodiscard]] const aabb &bounds() const {
    return is_face() ? m_face_geom.bounds : m_edge_geom.bounds;
  }

  /**
   * Ray-triangle intersection for face types.
   * @return t value (distance along ray) if hit, -1.0f if miss
   */
  [[nodiscard]] float intersect_ray(const glm::vec3 &ray_origin,
                                    const glm::vec3 &ray_dir) const;

  friend bool operator==(const subshape_handle &a, const subshape_handle &b) {
    return a.m_id == b.m_id;
  }
  friend bool operator!=(const subshape_handle &a, const subshape_handle &b) {
    return !(a == b);
  }

private:
  subshape_id m_id{invalid_subshape_id};

  // OCCT topology (only one is valid based on type)
  TopoDS_Face m_face;
  TopoDS_Edge m_edge;

  // Cached geometry
  face_geometry m_face_geom;
  edge_geometry m_edge_geom;

  void extract_face_geometry(const TopoDS_Face &face);
  void extract_edge_geometry(const TopoDS_Edge &edge);
};

/**
 * Hash function for subshape_handle.
 */
struct subshape_handle_hash {
  [[nodiscard]] std::size_t operator()(const subshape_handle &h) const noexcept {
    return subshape_id_hash{}(h.id());
  }
};

/**
 * Extract all faces and edges from a TopoDS_Shape.
 * @param shape The source BRep shape
 * @param out_faces Output vector for face handles
 * @param out_edges Output vector for edge handles
 */
void extract_subshapes(const TopoDS_Shape &shape,
                       std::vector<subshape_handle> &out_faces,
                       std::vector<subshape_handle> &out_edges);

} // namespace toy_cad::geometry
