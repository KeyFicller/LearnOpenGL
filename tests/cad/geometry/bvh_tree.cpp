#include "bvh_tree.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace toy_cad::geometry {

void bvh_tree::build(const std::vector<subshape_handle> &faces,
                     const std::vector<subshape_handle> &edges) {
  clear();

  m_faces = &faces;
  m_edges = &edges;

  // Build primitive list: faces first, then edges
  const uint32_t total_primitives = static_cast<uint32_t>(faces.size() + edges.size());
  if (total_primitives == 0) {
    return;
  }

  m_primitives.reserve(total_primitives);
  for (const auto &face : faces) {
    m_primitives.push_back(&face);
  }
  for (const auto &edge : edges) {
    m_primitives.push_back(&edge);
  }

  // Build index list for construction
  std::vector<uint32_t> indices(total_primitives);
  for (uint32_t i = 0; i < total_primitives; ++i) {
    indices[i] = i;
  }

  // Reserve nodes (worst case: 2n - 1 nodes for n leaves)
  m_nodes.reserve(total_primitives * 2);

  // Build recursively
  build_recursive(indices, 0, total_primitives);
}

void bvh_tree::clear() {
  m_nodes.clear();
  m_primitives.clear();
  m_faces = nullptr;
  m_edges = nullptr;
}

uint32_t bvh_tree::build_recursive(std::vector<uint32_t> &primitive_indices,
                                     uint32_t start,
                                     uint32_t end) {
  const uint32_t node_idx = static_cast<uint32_t>(m_nodes.size());
  m_nodes.emplace_back();
  bvh_node &node = m_nodes.back();

  // Compute bounds for this node
  node.bounds = compute_bounds(primitive_indices, start, end);

  const uint32_t count = end - start;

  // Leaf node: 1 primitive
  if (count == 1) {
    node.primitive_index = primitive_indices[start];
    return node_idx;
  }

  // Choose split axis (longest axis of bounds)
  const int split_axis = choose_split_axis(node.bounds);

  // Simple median split (fast and good enough for CAD)
  const uint32_t mid = start + count / 2;

  // Sort primitives by centroid along split axis
  std::nth_element(
      primitive_indices.begin() + start,
      primitive_indices.begin() + mid,
      primitive_indices.begin() + end,
      [this, split_axis](uint32_t a, uint32_t b) {
        const subshape_handle *ha = get_primitive(a);
        const subshape_handle *hb = get_primitive(b);
        if (!ha || !hb) return false;
        const glm::vec3 ca = ha->bounds().center();
        const glm::vec3 cb = hb->bounds().center();
        return ca[split_axis] < cb[split_axis];
      });

  // Recursively build children
  node.left_child = build_recursive(primitive_indices, start, mid);
  node.right_child = build_recursive(primitive_indices, mid, end);

  return node_idx;
}

aabb bvh_tree::compute_bounds(const std::vector<uint32_t> &primitive_indices,
                              uint32_t start,
                              uint32_t end) const {
  aabb bounds;
  bool first = true;

  for (uint32_t i = start; i < end; ++i) {
    const subshape_handle *handle = get_primitive(primitive_indices[i]);
    if (!handle) continue;

    const aabb &b = handle->bounds();
    if (!b.valid()) continue;

    if (first) {
      bounds = b;
      first = false;
    } else {
      bounds.expand(b);
    }
  }

  return bounds;
}

uint32_t bvh_tree::choose_split_axis(const aabb &bounds) const {
  const glm::vec3 extent = bounds.extent();
  if (extent.x > extent.y && extent.x > extent.z) return 0;
  if (extent.y > extent.z) return 1;
  return 2;
}

const subshape_handle *bvh_tree::get_primitive(uint32_t idx) const {
  if (idx < m_primitives.size()) {
    return m_primitives[idx];
  }
  return nullptr;
}

std::optional<ray_hit> bvh_tree::intersect_ray(const glm::vec3 &ray_origin,
                                                const glm::vec3 &ray_dir) const {
  if (m_nodes.empty()) {
    return std::nullopt;
  }

  // Stack-based traversal
  constexpr uint32_t MAX_STACK = 64;
  uint32_t stack[MAX_STACK];
  uint32_t stack_size = 0;

  // Push root
  stack[stack_size++] = 0;

  float closest_t = std::numeric_limits<float>::max();
  const subshape_handle *closest_handle = nullptr;

  while (stack_size > 0) {
    const uint32_t node_idx = stack[--stack_size];
    if (node_idx >= m_nodes.size()) continue;

    const bvh_node &node = m_nodes[node_idx];

    // Ray-box test
    if (!ray_aabb_intersect(node.bounds, ray_origin, ray_dir, 0.0f, closest_t)) {
      continue;
    }

    if (node.is_leaf()) {
      // Test primitive
      const subshape_handle *handle = get_primitive(node.primitive_index);
      if (handle) {
        const float t = handle->intersect_ray(ray_origin, ray_dir);
        if (t >= 0.0f && t < closest_t) {
          closest_t = t;
          closest_handle = handle;
        }
      }
    } else {
      // Push children (closer one first for better culling)
      if (node.left_child != 0) {
        stack[stack_size++] = node.left_child;
      }
      if (node.right_child != 0) {
        stack[stack_size++] = node.right_child;
      }
    }
  }

  if (closest_handle) {
    return ray_hit{closest_handle, closest_t};
  }
  return std::nullopt;
}

void bvh_tree::intersect_ray_all(const glm::vec3 &ray_origin,
                                 const glm::vec3 &ray_dir,
                                 std::vector<ray_hit> &out_hits) const {
  out_hits.clear();
  if (m_nodes.empty()) {
    return;
  }

  // Stack-based traversal
  constexpr uint32_t MAX_STACK = 64;
  uint32_t stack[MAX_STACK];
  uint32_t stack_size = 0;

  stack[stack_size++] = 0;

  while (stack_size > 0) {
    const uint32_t node_idx = stack[--stack_size];
    if (node_idx >= m_nodes.size()) continue;

    const bvh_node &node = m_nodes[node_idx];

    // Ray-box test
    if (!ray_aabb_intersect(node.bounds, ray_origin, ray_dir, 0.0f,
                            std::numeric_limits<float>::max())) {
      continue;
    }

    if (node.is_leaf()) {
      const subshape_handle *handle = get_primitive(node.primitive_index);
      if (handle) {
        const float t = handle->intersect_ray(ray_origin, ray_dir);
        if (t >= 0.0f) {
          out_hits.push_back(ray_hit{handle, t});
        }
      }
    } else {
      if (node.left_child != 0) {
        stack[stack_size++] = node.left_child;
      }
      if (node.right_child != 0) {
        stack[stack_size++] = node.right_child;
      }
    }
  }

  // Sort by distance
  std::sort(out_hits.begin(), out_hits.end(),
            [](const ray_hit &a, const ray_hit &b) { return a.t < b.t; });
}

void bvh_tree::intersect_frustum(const glm::vec4 frustum_planes[6],
                                 std::vector<const subshape_handle *> &out_hits) const {
  out_hits.clear();
  if (m_nodes.empty()) {
    return;
  }

  // Stack-based traversal
  constexpr uint32_t MAX_STACK = 64;
  uint32_t stack[MAX_STACK];
  uint32_t stack_size = 0;

  stack[stack_size++] = 0;

  while (stack_size > 0) {
    const uint32_t node_idx = stack[--stack_size];
    if (node_idx >= m_nodes.size()) continue;

    const bvh_node &node = m_nodes[node_idx];

    // Frustum-box test
    if (!aabb_frustum_intersect(node.bounds, frustum_planes)) {
      continue;
    }

    if (node.is_leaf()) {
      const subshape_handle *handle = get_primitive(node.primitive_index);
      if (handle) {
        out_hits.push_back(handle);
      }
    } else {
      if (node.left_child != 0) {
        stack[stack_size++] = node.left_child;
      }
      if (node.right_child != 0) {
        stack[stack_size++] = node.right_child;
      }
    }
  }
}

void bvh_tree::intersect_aabb(const aabb &box,
                              std::vector<const subshape_handle *> &out_hits) const {
  out_hits.clear();
  if (m_nodes.empty()) {
    return;
  }

  constexpr uint32_t MAX_STACK = 64;
  uint32_t stack[MAX_STACK];
  uint32_t stack_size = 0;

  stack[stack_size++] = 0;

  while (stack_size > 0) {
    const uint32_t node_idx = stack[--stack_size];
    if (node_idx >= m_nodes.size()) continue;

    const bvh_node &node = m_nodes[node_idx];

    if (!node.bounds.intersects(box)) {
      continue;
    }

    if (node.is_leaf()) {
      const subshape_handle *handle = get_primitive(node.primitive_index);
      if (handle) {
        out_hits.push_back(handle);
      }
    } else {
      if (node.left_child != 0) {
        stack[stack_size++] = node.left_child;
      }
      if (node.right_child != 0) {
        stack[stack_size++] = node.right_child;
      }
    }
  }
}

bool bvh_tree::ray_aabb_intersect(const aabb &box,
                                  const glm::vec3 &ray_origin,
                                  const glm::vec3 &ray_dir,
                                  float t_min,
                                  float t_max) {
  // Slab method
  for (int i = 0; i < 3; ++i) {
    const float inv_dir = 1.0f / ray_dir[i];
    float t1 = (box.min[i] - ray_origin[i]) * inv_dir;
    float t2 = (box.max[i] - ray_origin[i]) * inv_dir;

    if (inv_dir < 0.0f) {
      std::swap(t1, t2);
    }

    t_min = std::max(t_min, t1);
    t_max = std::min(t_max, t2);

    if (t_min > t_max) {
      return false;
    }
  }

  return t_max >= 0.0f;
}

bool bvh_tree::aabb_frustum_intersect(const aabb &box,
                                      const glm::vec4 frustum_planes[6]) {
  // Test box against each frustum plane
  for (int i = 0; i < 6; ++i) {
    const glm::vec3 normal = glm::vec3(frustum_planes[i]);
    const float distance = frustum_planes[i].w;

    // Find the box corner that is most opposite to the plane normal
    glm::vec3 p;
    p.x = (normal.x > 0.0f) ? box.min.x : box.max.x;
    p.y = (normal.y > 0.0f) ? box.min.y : box.max.y;
    p.z = (normal.z > 0.0f) ? box.min.z : box.max.z;

    // If the most opposite corner is outside, the box is outside
    if (glm::dot(p, normal) + distance < 0.0f) {
      return false;
    }
  }

  return true;
}

void frustum_from_screen_rect(const glm::vec2 &screen_min,
                              const glm::vec2 &screen_max,
                              const glm::vec2 &viewport_size,
                              const glm::mat4 &clip_from_world,
                              glm::vec4 out_planes[6]) {
  // Compute NDC coordinates
  const float ndc_min_x = (screen_min.x / viewport_size.x) * 2.0f - 1.0f;
  const float ndc_max_x = (screen_max.x / viewport_size.x) * 2.0f - 1.0f;
  const float ndc_min_y = 1.0f - (screen_max.y / viewport_size.y) * 2.0f;
  const float ndc_max_y = 1.0f - (screen_min.y / viewport_size.y) * 2.0f;

  // Get inverse transform
  const glm::mat4 world_from_clip = glm::inverse(clip_from_world);

  // Compute frustum corners in world space
  const glm::vec4 corners[8] = {
      world_from_clip * glm::vec4(ndc_min_x, ndc_min_y, -1.0f, 1.0f),
      world_from_clip * glm::vec4(ndc_max_x, ndc_min_y, -1.0f, 1.0f),
      world_from_clip * glm::vec4(ndc_min_x, ndc_max_y, -1.0f, 1.0f),
      world_from_clip * glm::vec4(ndc_max_x, ndc_max_y, -1.0f, 1.0f),
      world_from_clip * glm::vec4(ndc_min_x, ndc_min_y, 1.0f, 1.0f),
      world_from_clip * glm::vec4(ndc_max_x, ndc_min_y, 1.0f, 1.0f),
      world_from_clip * glm::vec4(ndc_min_x, ndc_max_y, 1.0f, 1.0f),
      world_from_clip * glm::vec4(ndc_max_x, ndc_max_y, 1.0f, 1.0f),
  };

  // Convert from homogeneous to 3D
  glm::vec3 pts[8];
  for (int i = 0; i < 8; ++i) {
    pts[i] = glm::vec3(corners[i]) / corners[i].w;
  }

  // Helper to compute plane from 3 points (returns vec4 with xyz=normal, w=distance)
  auto compute_plane = [](const glm::vec3 &a, const glm::vec3 &b,
                          const glm::vec3 &c) -> glm::vec4 {
    const glm::vec3 ab = b - a;
    const glm::vec3 ac = c - a;
    const glm::vec3 normal = glm::normalize(glm::cross(ab, ac));
    const float distance = -glm::dot(normal, a);
    return glm::vec4(normal, distance);
  };

  // Left plane (0, 2, 4, 6)
  out_planes[0] = compute_plane(pts[0], pts[4], pts[2]);
  // Right plane (1, 3, 5, 7)
  out_planes[1] = compute_plane(pts[1], pts[3], pts[5]);
  // Bottom plane (0, 1, 4, 5)
  out_planes[2] = compute_plane(pts[0], pts[1], pts[4]);
  // Top plane (2, 3, 6, 7)
  out_planes[3] = compute_plane(pts[2], pts[6], pts[3]);
  // Near plane (0, 1, 2, 3)
  out_planes[4] = compute_plane(pts[0], pts[2], pts[1]);
  // Far plane (4, 5, 6, 7)
  out_planes[5] = compute_plane(pts[4], pts[5], pts[6]);
}

} // namespace toy_cad::geometry
