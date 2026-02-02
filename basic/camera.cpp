#include "basic/camera.h"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/geometric.hpp"
#include <cmath>

void camera::update_view_matrix() {
  // Recalculate front vector from yaw and pitch
  glm::vec3 front;
  front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
  front.y = sin(glm::radians(Pitch));
  front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
  Front = glm::normalize(front);

  // Recalculate right vector (used to compute up vector)
  glm::vec3 right_vec = glm::normalize(glm::cross(WorldUp, Front));
  // Recalculate up vector (perpendicular to both front and right)
  glm::vec3 up_vec = glm::normalize(glm::cross(Front, right_vec));

  ViewMatrix = glm::lookAt(Position, Position + Front, up_vec);
}

void camera::update_projection_matrix() {
  if (Orthographic) {
    ProjectionMatrix = glm::ortho(Left, Right, Bottom, Top, Near, Far);
  } else {
    ProjectionMatrix =
        glm::perspective(glm::radians(FOV), AspectRatio, Near, Far);
  }
}

void camera::set_aspect_ratio(float _aspect_ratio) {
  AspectRatio = _aspect_ratio;
  update_projection_matrix();
}

glm::vec3 camera::right() const {
  return glm::normalize(glm::cross(WorldUp, Front));
}

glm::vec3 camera::up() const {
  return glm::normalize(glm::cross(Front, right()));
}

camera::camera() {
  update_view_matrix();
  update_projection_matrix();
}
