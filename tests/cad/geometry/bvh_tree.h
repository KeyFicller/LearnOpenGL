#pragma once

#include "subshape_handle.h"
#include "tests/cad/database/database.h"
#include "tests/cad/database/handle.h"

#include <glm/glm.hpp>

#include <functional>
#include <vector>
#include <optional>

namespace toy_cad::geometry {

struct bvh_node {
  aabb bounds;
  uint32_t left_child = 0;
  uint32_t right_child = 0;
  uint32_t primitive_index = 0xffffffffu;

  bool is_leaf() const {
    return left_child == 0 && right_child == 0;
  }
};

struct ray_hit {
  handle subshape{};
  float t = -1.0f;

  bool valid() const { return subshape.valid() && t >= 0.0f; }
};

using bvh_bounds_fn = std::function<const aabb *(handle)>;
using bvh_intersect_ray_fn =
    std::function<float(handle, const glm::vec3 &, const glm::vec3 &)>;

class bvh_tree {
public:
  bvh_tree() = default;
  ~bvh_tree() = default;

  bvh_tree(const bvh_tree &) = delete;
  bvh_tree &operator=(const bvh_tree &) = delete;
  bvh_tree(bvh_tree &&) noexcept = default;
  bvh_tree &operator=(bvh_tree &&) noexcept = default;

  void build(const std::vector<handle> &subshape_handles,
             bvh_bounds_fn bounds_fn);

  void clear();

  bool empty() const { return m_nodes.empty(); }

  [[nodiscard]] std::optional<ray_hit>
  intersect_ray(const glm::vec3 &ray_origin, const glm::vec3 &ray_dir,
                bvh_intersect_ray_fn intersect_fn) const;

  void intersect_ray_all(const glm::vec3 &ray_origin, const glm::vec3 &ray_dir,
                         bvh_intersect_ray_fn intersect_fn,
                         std::vector<ray_hit> &out_hits) const;

  void intersect_frustum(const glm::vec4 frustum_planes[6],
                         std::vector<handle> &out_hits) const;

  void intersect_aabb(const aabb &box, std::vector<handle> &out_hits) const;

private:
  std::vector<bvh_node> m_nodes;
  std::vector<handle> m_primitives;
  bvh_bounds_fn m_bounds_fn;

  uint32_t build_recursive(std::vector<uint32_t> &primitive_indices,
                           uint32_t start, uint32_t end);

  aabb compute_bounds(const std::vector<uint32_t> &primitive_indices,
                                    uint32_t start, uint32_t end) const;

  uint32_t choose_split_axis(const aabb &bounds) const;

  [[nodiscard]] static bool ray_aabb_intersect(const aabb &box,
                                               const glm::vec3 &ray_origin,
                                               const glm::vec3 &ray_dir,
                                               float t_min, float t_max);

  [[nodiscard]] static bool aabb_frustum_intersect(const aabb &box,
                                                   const glm::vec4 frustum_planes[6]);

  handle get_primitive(uint32_t idx) const;
  const aabb *bounds_for(uint32_t primitive_idx) const;
};

void frustum_from_screen_rect(const glm::vec2 &screen_min,
                              const glm::vec2 &screen_max,
                              const glm::vec2 &viewport_size,
                              const glm::mat4 &clip_from_world,
                              glm::vec4 out_planes[6]);

} // namespace toy_cad::geometry
