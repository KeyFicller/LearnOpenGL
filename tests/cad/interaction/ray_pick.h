#pragma once

#include "tests/cad/renderer/disp_var.h"
#include <glm/glm.hpp>

namespace toy_cad::interaction {

/**
 * Ray casting utility for 3D point picking from screen coordinates.
 *
 * Default working plane: passes through world origin, perpendicular to view ray.
 * This means the plane normal aligns with the camera's view direction.
 */
class ray_pick {
public:
  static ray_pick &instance();

  ray_pick(const ray_pick &) = delete;
  ray_pick &operator=(const ray_pick &) = delete;
  ray_pick(ray_pick &&) noexcept = default;
  ray_pick &operator=(ray_pick &&) noexcept = default;

  /** Ray in world space. */
  struct ray {
    glm::vec3 origin;    // Camera position
    glm::vec3 direction; // Normalized ray direction
  };

  /**
   * Generate world-space ray from screen coordinates.
   * @param screen_x Mouse X in screen pixels (0 = left)
   * @param screen_y Mouse Y in screen pixels (0 = top)
   * @param disp Display/camera state for matrix lookup
   */
  [[nodiscard]] ray screen_to_world_ray(double screen_x, double screen_y,
                                          const disp_var &disp) const;

  /**
   * Intersect ray with default working plane (through origin, perpendicular to ray direction).
   * Returns true if intersection exists, false if ray is parallel to plane.
   */
  [[nodiscard]] bool intersect_working_plane(const ray &r, glm::vec3 &out_point) const;

  /**
   * Pick 3D point at screen coordinates using default working plane.
   * Returns true if successful, false if ray is parallel to plane.
   */
  [[nodiscard]] bool pick_at(double screen_x, double screen_y,
                             const disp_var &disp, glm::vec3 &out_point) const;

  /**
   * Pick with custom working plane defined by origin and normal.
   */
  [[nodiscard]] bool pick_on_plane(double screen_x, double screen_y,
                                   const disp_var &disp,
                                   const glm::vec3 &plane_origin,
                                   const glm::vec3 &plane_normal,
                                   glm::vec3 &out_point) const;

  /**
   * Ray-triangle intersection using Möller-Trumbore algorithm.
   * @param ray_origin Ray origin
   * @param ray_dir Ray direction (normalized)
   * @param v0 Triangle vertex 0
   * @param v1 Triangle vertex 1
   * @param v2 Triangle vertex 2
   * @param out_t Output: distance along ray to intersection point (if any)
   * @param out_u Output: barycentric coordinate u (if any)
   * @param out_v Output: barycentric coordinate v (if any)
   * @return true if ray intersects triangle, false otherwise
   */
  [[nodiscard]] static bool ray_triangle_intersect(const glm::vec3 &ray_origin,
                                                   const glm::vec3 &ray_dir,
                                                   const glm::vec3 &v0,
                                                   const glm::vec3 &v1,
                                                   const glm::vec3 &v2,
                                                   float &out_t,
                                                   float &out_u,
                                                   float &out_v);

  /**
   * Ray-axis-aligned bounding box intersection using slab method.
   * @param ray_origin Ray origin
   * @param ray_dir Ray direction (normalized)
   * @param box_min AABB minimum corner
   * @param box_max AABB maximum corner
   * @param out_t_min Output: entry distance
   * @param out_t_max Output: exit distance
   * @return true if ray intersects AABB
   */
  [[nodiscard]] static bool ray_aabb_intersect(const glm::vec3 &ray_origin,
                                               const glm::vec3 &ray_dir,
                                               const glm::vec3 &box_min,
                                               const glm::vec3 &box_max,
                                               float &out_t_min,
                                               float &out_t_max);

  /**
   * Construct a view frustum from screen rectangle for box selection.
   * @param screen_min Top-left corner of selection rect (pixels)
   * @param screen_max Bottom-right corner of selection rect (pixels)
   * @param disp Display/camera state
   * @param out_planes Output: 6 frustum plane equations (xyz = normal, w = distance)
   */
  void frustum_from_screen_rect(const glm::vec2 &screen_min,
                                const glm::vec2 &screen_max,
                                const disp_var &disp,
                                glm::vec4 out_planes[6]) const;

private:
  ray_pick() = default;
  ~ray_pick() = default;
};

} // namespace toy_cad::interaction
