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
  glm::vec3 m_up = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 m_front = glm::normalize(glm::vec3(0.0f, 0.0f, 0.0f) - m_position);
  float m_fov = 45.0f;
  float m_aspect_ratio = 1.0f;
  float m_near = 0.1f;
  float m_far = 100.0f;

public:
  void update_view_matrix() {
    m_view_matrix = glm::lookAt(m_position, look_at(), m_up);
  }
  void update_projection_matrix() {
    m_projection_matrix =
        glm::perspective(glm::radians(m_fov), m_aspect_ratio, m_near, m_far);
  }
  glm::mat4 m_view_matrix;
  glm::mat4 m_projection_matrix;

public:
  glm::vec3 look_at() const { return m_position + m_front; }
  glm::vec3 front() const { return m_front; }
  glm::vec3 right() const { return glm::normalize(glm::cross(m_up, front())); }
  glm::vec3 up() const { return m_up; }

public:
  camera() {
    update_view_matrix();
    update_projection_matrix();
  }
};