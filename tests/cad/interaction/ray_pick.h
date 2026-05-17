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

private:
  ray_pick() = default;
  ~ray_pick() = default;
};

} // namespace toy_cad::interaction
