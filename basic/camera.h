#pragma once

#include "glm/glm.hpp"

/**
 * @brief Camera
 */
struct camera {
  // Position and orientation
  glm::vec3 Position = glm::vec3(0.0f, 0.0f, 3.0f);
  glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 Front = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));

  // Projection parameters
  float FOV = 45.0f;
  float AspectRatio = 1.0f;
  float Near = 0.1f;
  float Far = 100.0f;

  // Yaw and pitch angles (in degrees)
  float Yaw = -90.0f;   // Default looking along -Z axis
  float Pitch = 0.0f;

  // Projection type
  bool Orthographic = false;

  // Matrices (updated by update methods)
  glm::mat4 ViewMatrix;
  glm::mat4 ProjectionMatrix;

public:
  /**
   * @brief Update view matrix from position and orientation
   */
  void update_view_matrix();

  /**
   * @brief Update projection matrix from projection parameters
   */
  void update_projection_matrix();

  /**
   * @brief Set aspect ratio and update projection matrix
   * @param _aspect_ratio Aspect ratio to set
   */
  void set_aspect_ratio(float _aspect_ratio);

  /**
   * @brief Get look-at point (position + front)
   * @return Look-at point
   */
  glm::vec3 look_at() const { return Position + Front; }

  /**
   * @brief Get front vector
   * @return Front vector
   */
  glm::vec3 front() const { return Front; }

  /**
   * @brief Get right vector
   * @return Right vector
   */
  glm::vec3 right() const;

  /**
   * @brief Get up vector
   * @return Up vector
   */
  glm::vec3 up() const;

public:
  /**
   * @brief Construct camera and initialize matrices
   */
  camera();
};
