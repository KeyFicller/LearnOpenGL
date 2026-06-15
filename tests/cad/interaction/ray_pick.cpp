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

bool ray_pick::ray_triangle_intersect(const glm::vec3 &ray_origin,
                                      const glm::vec3 &ray_dir,
                                      const glm::vec3 &v0,
                                      const glm::vec3 &v1,
                                      const glm::vec3 &v2,
                                      float &out_t,
                                      float &out_u,
                                      float &out_v) {
  // Möller-Trumbore algorithm
  constexpr float EPSILON = 1e-6f;

  const glm::vec3 edge1 = v1 - v0;
  const glm::vec3 edge2 = v2 - v0;
  const glm::vec3 h = glm::cross(ray_dir, edge2);
  const float a = glm::dot(edge1, h);

  if (std::abs(a) < EPSILON) {
    return false; // Ray parallel to triangle
  }

  const float f = 1.0f / a;
  const glm::vec3 s = ray_origin - v0;
  out_u = f * glm::dot(s, h);

  if (out_u < 0.0f || out_u > 1.0f) {
    return false;
  }

  const glm::vec3 q = glm::cross(s, edge1);
  out_v = f * glm::dot(ray_dir, q);

  if (out_v < 0.0f || out_u + out_v > 1.0f) {
    return false;
  }

  out_t = f * glm::dot(edge2, q);

  return out_t > EPSILON;
}

bool ray_pick::ray_aabb_intersect(const glm::vec3 &ray_origin,
                                  const glm::vec3 &ray_dir,
                                  const glm::vec3 &box_min,
                                  const glm::vec3 &box_max,
                                  float &out_t_min,
                                  float &out_t_max) {
  // Slab method for ray-AABB intersection
  out_t_min = 0.0f;
  out_t_max = std::numeric_limits<float>::max();

  for (int i = 0; i < 3; ++i) {
    const float inv_dir = 1.0f / ray_dir[i];
    float t1 = (box_min[i] - ray_origin[i]) * inv_dir;
    float t2 = (box_max[i] - ray_origin[i]) * inv_dir;

    if (inv_dir < 0.0f) {
      std::swap(t1, t2);
    }

    out_t_min = std::max(out_t_min, t1);
    out_t_max = std::min(out_t_max, t2);

    if (out_t_min > out_t_max) {
      return false;
    }
  }

  return out_t_max >= 0.0f;
}

void ray_pick::frustum_from_screen_rect(const glm::vec2 &screen_min,
                                        const glm::vec2 &screen_max,
                                        const disp_var &disp,
                                        glm::vec4 out_planes[6]) const {
  const glm::vec2 viewport_size(static_cast<float>(disp.render_width),
                                 static_cast<float>(disp.render_height));

  // Compute NDC coordinates
  const float ndc_min_x = (screen_min.x / viewport_size.x) * 2.0f - 1.0f;
  const float ndc_max_x = (screen_max.x / viewport_size.x) * 2.0f - 1.0f;
  const float ndc_min_y = 1.0f - (screen_max.y / viewport_size.y) * 2.0f;
  const float ndc_max_y = 1.0f - (screen_min.y / viewport_size.y) * 2.0f;

  // Get world-to-clip and invert
  const glm::mat4 clip_from_world = disp.clip_from_world();
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

  // Helper to compute plane from 3 points
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

} // namespace toy_cad::interaction
