#pragma once

#include "subshape_handle.h"

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <optional>

namespace toy_cad::geometry {

/**
 * Axis-aligned bounding volume hierarchy node.
 * Used for fast spatial queries (ray casting, frustum intersection).
 */
struct bvh_node {
  aabb bounds;

  // For internal nodes: children indices (0 = invalid/null)
  uint32_t left_child = 0;
  uint32_t right_child = 0;

  // For leaf nodes: index into the primitive array
  uint32_t primitive_index = 0xffffffffu;

  [[nodiscard]] bool is_leaf() const { return left_child == 0 && right_child == 0; }
};

/**
 * Ray-bounding box intersection result.
 */
struct ray_hit {
  const subshape_handle *handle = nullptr;
  float t = -1.0f;  // Distance along ray

  [[nodiscard]] bool valid() const { return handle != nullptr && t >= 0.0f; }
};

/**
 * Bounding Volume Hierarchy tree for subshape_handles.
 * Provides O(log n) ray intersection and frustum queries.
 */
class bvh_tree {
public:
  bvh_tree() = default;
  ~bvh_tree() = default;

  bvh_tree(const bvh_tree &) = delete;
  bvh_tree &operator=(const bvh_tree &) = delete;
  bvh_tree(bvh_tree &&) noexcept = default;
  bvh_tree &operator=(bvh_tree &&) noexcept = default;

  /**
   * Build the BVH from a list of subshapes.
   * Clears any existing tree and builds a new one.
   */
  void build(const std::vector<subshape_handle> &faces,
             const std::vector<subshape_handle> &edges);

  /**
   * Clear the tree.
   */
  void clear();

  [[nodiscard]] bool empty() const { return m_nodes.empty(); }

  /**
   * Intersect a ray with the BVH, returning the closest hit.
   * @param ray_origin Ray origin in world space
   * @param ray_dir Normalized ray direction
   * @return Optional hit information
   */
  [[nodiscard]] std::optional<ray_hit> intersect_ray(const glm::vec3 &ray_origin,
                                                      const glm::vec3 &ray_dir) const;

  /**
   * Intersect a ray with the BVH, returning all hits (sorted by distance).
   * Useful for transparency or multi-layer picking.
   */
  void intersect_ray_all(const glm::vec3 &ray_origin,
                         const glm::vec3 &ray_dir,
                         std::vector<ray_hit> &out_hits) const;

  /**
   * Find all subshapes whose bounds intersect with a frustum.
   * Used for box selection.
   * @param frustum_planes 6 plane equations (xyz = normal, w = distance)
   */
  void intersect_frustum(const glm::vec4 frustum_planes[6],
                           std::vector<const subshape_handle *> &out_hits) const;

  /**
   * Find all subshapes whose bounds intersect with an axis-aligned bounding box.
   * Used for spatial queries.
   */
  void intersect_aabb(const aabb &box,
                      std::vector<const subshape_handle *> &out_hits) const;

private:
  // Flat array of nodes (root at index 0)
  std::vector<bvh_node> m_nodes;

  // Flat array of primitive handles (faces first, then edges)
  std::vector<const subshape_handle *> m_primitives;

  // Reference to source data (non-owning)
  const std::vector<subshape_handle> *m_faces = nullptr;
  const std::vector<subshape_handle> *m_edges = nullptr;

  // Build helper methods
  uint32_t build_recursive(std::vector<uint32_t> &primitive_indices,
                           uint32_t start,
                           uint32_t end);

  [[nodiscard]] aabb compute_bounds(const std::vector<uint32_t> &primitive_indices,
                                    uint32_t start,
                                    uint32_t end) const;

  [[nodiscard]] uint32_t choose_split_axis(const aabb &bounds) const;

  [[nodiscard]] float evaluate_split_cost(const std::vector<uint32_t> &primitive_indices,
                                          uint32_t start,
                                          uint32_t end,
                                          int axis,
                                          float split_pos) const;

  // Ray-node intersection
  [[nodiscard]] static bool ray_aabb_intersect(const aabb &box,
                                               const glm::vec3 &ray_origin,
                                               const glm::vec3 &ray_dir,
                                               float t_min,
                                               float t_max);

  // Frustum-node intersection
  [[nodiscard]] static bool aabb_frustum_intersect(const aabb &box,
                                                   const glm::vec4 frustum_planes[6]);

  // Get primitive by index
  [[nodiscard]] const subshape_handle *get_primitive(uint32_t idx) const;
};

/**
 * Construct a view frustum from screen space selection rectangle.
 * @param screen_min Top-left corner of selection rect (pixels)
 * @param screen_max Bottom-right corner of selection rect (pixels)
 * @param viewport_size Total viewport dimensions (pixels)
 * @param clip_from_world Clip-space transformation matrix
 * @param out_planes Output array of 6 plane equations (xyz = normal, w = distance)
 */
void frustum_from_screen_rect(const glm::vec2 &screen_min,
                              const glm::vec2 &screen_max,
                              const glm::vec2 &viewport_size,
                              const glm::mat4 &clip_from_world,
                              glm::vec4 out_planes[6]);

} // namespace toy_cad::geometry
