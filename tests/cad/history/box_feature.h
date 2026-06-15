#pragma once

#include "primitive_feature.h"
#include "tests/cad/geometry/subshape_handle.h"
#include "tests/cad/geometry/bvh_tree.h"
#include <glm/glm.hpp>

#include <BRepBuilderAPI_MakeSolid.hxx>
#include <TopoDS_Shape.hxx>
#include <memory>

namespace toy_cad {

/**
 * Axis-aligned box primitive.
 * Stores two diagonal corners, generates OCCT topology on regen().
 * Supports subshape (face/edge) selection via BVH acceleration.
 */
class box_feature : public primitive_feature {
public:
  box_feature() = default;

  /** Set corner points (axis-aligned, opposite corners). */
  void set_corners(const glm::vec3 &p1, const glm::vec3 &p2);
  [[nodiscard]] glm::vec3 corner_min() const { return m_min; }
  [[nodiscard]] glm::vec3 corner_max() const { return m_max; }
  [[nodiscard]] bool has_geometry() const { return m_shape_ok; }

  /** Rebuild OCCT topology from current corners. */
  void regen() override;

  void draw_global() override;
  void draw_local() override;
  void draw_ui(handle explorer_row) override;
  void inspect() const override;

  /** Selection support */
  [[nodiscard]] const geometry::bvh_tree &bvh() const { return m_bvh; }
  [[nodiscard]] bool has_bvh() const { return !m_bvh.empty(); }

  /**
   * Ray intersection for picking.
   * @param ray_origin Ray origin in world space
   * @param ray_dir Ray direction (normalized)
   * @return Optional hit info with subshape handle and distance
   */
  [[nodiscard]] std::optional<geometry::ray_hit> intersect_ray(
      const glm::vec3 &ray_origin, const glm::vec3 &ray_dir) const;

  /**
   * Frustum intersection for box selection.
   * @param frustum_planes 6 plane equations (xyz = normal, w = distance)
   */
  void intersect_frustum(const glm::vec4 frustum_planes[6],
                         std::vector<const geometry::subshape_handle *> &out_hits) const;

  /**
   * Draw a specific subshape in highlight color (for selection/hover).
   */
  void draw_subshape_highlighted(const geometry::subshape_handle &handle,
                                 const glm::vec3 &color) const;

  /**
   * Draw a list of subshapes in highlight color.
   */
  void draw_subshapes_highlighted(
      const std::vector<const geometry::subshape_handle *> &handles,
      const glm::vec3 &color) const;

  /** Access to subshapes for selection management */
  [[nodiscard]] const std::vector<geometry::subshape_handle> &faces() const { return m_faces; }
  [[nodiscard]] const std::vector<geometry::subshape_handle> &edges() const { return m_edges; }

private:
  glm::vec3 m_min{0.0f, 0.0f, 0.0f};
  glm::vec3 m_max{0.0f, 0.0f, 0.0f};

  // OCCT topology
  TopoDS_Shape m_shape;
  bool m_shape_ok = false;

  // OpenGL render data (mesh triangulation)
  struct render_mesh {
    std::vector<float> vertices; // position (3 floats per vertex)
    std::vector<float> normals;  // normal (3 floats per vertex)
    std::vector<unsigned int> indices;
    unsigned int vao = 0, vbo = 0, nbo = 0, ibo = 0;
    bool dirty = true;
  };
  mutable render_mesh m_mesh;

  // Subshape data for selection
  std::vector<geometry::subshape_handle> m_faces;
  std::vector<geometry::subshape_handle> m_edges;
  geometry::bvh_tree m_bvh;

  void triangulate_shape() const;
  void upload_mesh() const;
  void draw_mesh() const;

  void build_subshape_data();
  void draw_single_face(const geometry::subshape_handle &face_handle,
                        const glm::vec3 &color) const;
  void draw_single_edge(const geometry::subshape_handle &edge_handle,
                        const glm::vec3 &color) const;
};

} // namespace toy_cad
