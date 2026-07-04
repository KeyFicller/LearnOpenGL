#include "fit_view.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace toy_cad {

namespace {

std::array<glm::vec3, 8> aabb_corners(const geometry::aabb &bounds) {
  return {{
      {bounds.min.x, bounds.min.y, bounds.min.z},
      {bounds.max.x, bounds.min.y, bounds.min.z},
      {bounds.max.x, bounds.max.y, bounds.min.z},
      {bounds.min.x, bounds.max.y, bounds.min.z},
      {bounds.min.x, bounds.min.y, bounds.max.z},
      {bounds.max.x, bounds.min.y, bounds.max.z},
      {bounds.max.x, bounds.max.y, bounds.max.z},
      {bounds.min.x, bounds.max.y, bounds.max.z},
  }};
}

void fit_orthographic(camera &cam, const geometry::aabb &bounds,
                      float aspect_ratio) {
  constexpr float k_padding = 1.15f;
  const glm::vec3 center = bounds.center();
  const glm::vec3 right = cam.right();
  const glm::vec3 up = cam.up();
  const glm::vec3 forward = cam.front();

  float half_w = 1e-4f;
  float half_h = 1e-4f;
  for (const glm::vec3 &corner : aabb_corners(bounds)) {
    const glm::vec3 rel = corner - center;
    half_w = std::max(half_w, std::abs(glm::dot(rel, right)));
    half_h = std::max(half_h, std::abs(glm::dot(rel, up)));
  }
  half_w *= k_padding;
  half_h *= k_padding;

  if (aspect_ratio > 1e-6f) {
    if (half_w / half_h > aspect_ratio) {
      half_h = half_w / aspect_ratio;
    } else {
      half_w = half_h * aspect_ratio;
    }
  }

  const float standoff =
      std::max(glm::length(bounds.extent()) * 2.0f, 5.0f);
  cam.Position = center - forward * standoff;
  cam.Left = -half_w;
  cam.Right = half_w;
  cam.Bottom = -half_h;
  cam.Top = half_h;
  cam.update_view_matrix();
  cam.update_projection_matrix();
}

void fit_perspective(camera &cam, const geometry::aabb &bounds,
                     float aspect_ratio) {
  constexpr float k_padding = 1.15f;
  const glm::vec3 center = bounds.center();
  const glm::vec3 right = cam.right();
  const glm::vec3 up = cam.up();
  const glm::vec3 forward = cam.front();

  float half_w = 1e-4f;
  float half_h = 1e-4f;
  for (const glm::vec3 &corner : aabb_corners(bounds)) {
    const glm::vec3 rel = corner - center;
    half_w = std::max(half_w, std::abs(glm::dot(rel, right)));
    half_h = std::max(half_h, std::abs(glm::dot(rel, up)));
  }
  half_w *= k_padding;
  half_h *= k_padding;

  const float vfov = glm::radians(cam.FOV);
  const float tan_half_v = std::tan(vfov * 0.5f);
  const float tan_half_h = tan_half_v * std::max(aspect_ratio, 1e-6f);

  const float dist_v = half_h / std::max(tan_half_v, 1e-6f);
  const float dist_h = half_w / std::max(tan_half_h, 1e-6f);
  const float dist = std::max({dist_v, dist_h, 0.5f});

  cam.Position = center - forward * dist;
  cam.update_view_matrix();
}

} // namespace

void fit_camera_to_bounds(camera &cam, const geometry::aabb &bounds,
                          float aspect_ratio) {
  if (!bounds.valid()) {
    return;
  }

  if (cam.Orthographic) {
    fit_orthographic(cam, bounds, aspect_ratio);
  } else {
    fit_perspective(cam, bounds, aspect_ratio);
  }
}

} // namespace toy_cad
