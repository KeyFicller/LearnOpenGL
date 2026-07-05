#include "sketch_camera.h"

#include "tests/cad/camera/fit_view.h"

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <algorithm>
#include <cmath>

namespace toy_cad {

namespace {

glm::vec3 to_glm(const gp_Pnt &p) {
  return glm::vec3(static_cast<float>(p.X()), static_cast<float>(p.Y()),
                   static_cast<float>(p.Z()));
}

glm::vec3 to_glm(const gp_Dir &d) {
  return glm::vec3(static_cast<float>(d.X()), static_cast<float>(d.Y()),
                   static_cast<float>(d.Z()));
}

} // namespace

void align_camera_to_sketch_plane(
    camera &cam, const gp_Ax2 &plane, float aspect_ratio,
    const std::optional<geometry::aabb> &fit_bounds) {
  const glm::vec3 origin = to_glm(plane.Location());
  const glm::vec3 normal = glm::normalize(to_glm(plane.Direction()));
  const glm::vec3 x_dir = glm::normalize(to_glm(plane.XDirection()));
  const glm::vec3 y_dir = glm::normalize(glm::cross(normal, x_dir));

  constexpr float k_view_distance = 10.0f;
  constexpr float k_half_extent = 5.0f;

  cam.Orthographic = true;
  cam.WorldUp = y_dir;

  // Match camera::update_view_matrix (Pitch = asin(front.y), Yaw = atan2(front.z, front.x)).
  const glm::vec3 look_dir = -normal;
  cam.Pitch = std::asin(std::clamp(look_dir.y, -1.0f, 1.0f)) * 180.0f /
              static_cast<float>(M_PI);
  cam.Yaw = std::atan2(look_dir.z, look_dir.x) * 180.0f /
            static_cast<float>(M_PI);
  cam.update_view_matrix();

  if (fit_bounds.has_value() && fit_bounds->valid()) {
    fit_camera_to_bounds(cam, *fit_bounds, aspect_ratio);
    return;
  }

  cam.Position = origin + normal * k_view_distance;

  float half_w = k_half_extent;
  float half_h = k_half_extent;
  if (aspect_ratio > 1e-6f) {
    if (half_w / half_h > aspect_ratio) {
      half_h = half_w / aspect_ratio;
    } else {
      half_w = half_h * aspect_ratio;
    }
  }

  cam.Left = -half_w;
  cam.Right = half_w;
  cam.Bottom = -half_h;
  cam.Top = half_h;

  if (aspect_ratio > 1e-6f) {
    cam.set_aspect_ratio(aspect_ratio);
  }
  cam.update_view_matrix();
  cam.update_projection_matrix();
}

} // namespace toy_cad
