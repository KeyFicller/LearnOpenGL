#include "ray_pick.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace toy_cad::interaction {

ray_pick &ray_pick::instance() {
  static ray_pick s;
  return s;
}

ray_pick::ray ray_pick::screen_to_world_ray(double screen_x, double screen_y,
                                            const disp_var &disp) const {
  const int w = disp.render_width;
  const int h = disp.render_height;

  // If viewport not set, return forward ray from camera
  if (w <= 0 || h <= 0) {
    const glm::vec3 forward = -glm::normalize(
        glm::vec3(disp.view_matrix[0][2], disp.view_matrix[1][2], disp.view_matrix[2][2]));
    return {disp.camera_world_position, forward};
  }

  // Screen coords (top-left origin) to normalized device coords (NDC)
  // NDC: x,y in [-1, 1], z = -1 for near plane
  const float ndc_x = (static_cast<float>(screen_x) / static_cast<float>(w)) * 2.0f - 1.0f;
  const float ndc_y = 1.0f - (static_cast<float>(screen_y) / static_cast<float>(h)) * 2.0f;

  // Build inverse VP matrix
  const glm::mat4 vp = disp.clip_from_world();
  const glm::mat4 inv_vp = glm::inverse(vp);

  // Unproject near and far points
  const glm::vec4 near_p = inv_vp * glm::vec4(ndc_x, ndc_y, -1.0f, 1.0f);
  const glm::vec4 far_p = inv_vp * glm::vec4(ndc_x, ndc_y, 1.0f, 1.0f);

  // Perspective divide
  const glm::vec3 near_world = near_p.w != 0.0f ? glm::vec3(near_p) / near_p.w : glm::vec3(near_p);
  const glm::vec3 far_world = far_p.w != 0.0f ? glm::vec3(far_p) / far_p.w : glm::vec3(far_p);

  ray r;
  r.origin = near_world;
  r.direction = glm::normalize(far_world - near_world);
  return r;
}

bool ray_pick::intersect_working_plane(const ray &r, glm::vec3 &out_point) const {
  // Default working plane: through origin, perpendicular to ray direction
  // This means the plane normal = ray direction (plane faces the ray origin)
  const glm::vec3 plane_origin(0.0f, 0.0f, 0.0f);
  const glm::vec3 plane_normal = r.direction; // Perpendicular to view direction

  const float denom = glm::dot(r.direction, plane_normal);
  if (std::abs(denom) < 1e-6f) {
    return false; // Ray parallel to plane
  }

  const float t = glm::dot(plane_origin - r.origin, plane_normal) / denom;
  // Allow intersection behind camera for working plane mode
  out_point = r.origin + r.direction * t;
  return true;
}

bool ray_pick::pick_at(double screen_x, double screen_y,
                       const disp_var &disp, glm::vec3 &out_point) const {
  const ray r = screen_to_world_ray(screen_x, screen_y, disp);
  return intersect_working_plane(r, out_point);
}

bool ray_pick::pick_on_plane(double screen_x, double screen_y,
                             const disp_var &disp,
                             const glm::vec3 &plane_origin,
                             const glm::vec3 &plane_normal,
                             glm::vec3 &out_point) const {
  const ray r = screen_to_world_ray(screen_x, screen_y, disp);

  const float denom = glm::dot(r.direction, plane_normal);
  if (std::abs(denom) < 1e-6f) {
    return false; // Ray parallel to plane
  }

  const float t = glm::dot(plane_origin - r.origin, plane_normal) / denom;
  out_point = r.origin + r.direction * t;
  return true;
}

} // namespace toy_cad::interaction
