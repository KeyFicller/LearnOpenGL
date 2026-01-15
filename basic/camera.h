#pragma once

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/glm.hpp"

struct camera {
public:
  glm::vec3 m_position = glm::vec3(0.0f, 0.0f, 3.0f);
  glm::vec3 m_world_up = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 m_front = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
  float m_fov = 45.0f;
  float m_aspect_ratio = 1.0f;
  float m_near = 0.1f;
  float m_far = 100.0f;

  // Yaw and pitch angles (in degrees)
  float m_yaw = -90.0f; // Default looking along -Z axis
  float m_pitch = 0.0f;

public:
  void update_view_matrix() {
    // Recalculate front vector from yaw and pitch
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);

    // Recalculate right and up vectors
    glm::vec3 right = glm::normalize(glm::cross(m_world_up, m_front));
    glm::vec3 up = glm::normalize(glm::cross(m_front, right));

    m_view_matrix = glm::lookAt(m_position, m_position + m_front, up);
  }

  void update_projection_matrix() {
    m_projection_matrix =
        glm::perspective(glm::radians(m_fov), m_aspect_ratio, m_near, m_far);
  }

  void set_aspect_ratio(float _aspect_ratio) {
    m_aspect_ratio = _aspect_ratio;
    update_projection_matrix();
  }

  glm::mat4 m_view_matrix;
  glm::mat4 m_projection_matrix;

public:
  glm::vec3 look_at() const { return m_position + m_front; }
  glm::vec3 front() const { return m_front; }
  glm::vec3 right() const {
    return glm::normalize(glm::cross(m_world_up, m_front));
  }
  glm::vec3 up() const { return glm::normalize(glm::cross(m_front, right())); }

public:
  camera() {
    update_view_matrix();
    update_projection_matrix();
  }
};