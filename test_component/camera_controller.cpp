#include "camera_controller.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

namespace {
const float sensitivity = 0.1f;
const float max_pitch_angle = 89.0f;   // degrees
const float scroll_sensitivity = 2.0f; // FOV change per scroll unit
const float min_fov = 1.0f;            // degrees
const float max_fov = 120.0f;          // degrees
} // namespace

camera_controller::camera_controller(camera &_camera, GLFWwindow *window)
    : m_camera(_camera), m_window(window) {}

void camera_controller::update(float _delta_time) {
  float speed = 2.5f * _delta_time;
  if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) {
    m_camera.m_position += m_camera.front() * speed;
  }
  if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) {
    m_camera.m_position -= m_camera.front() * speed;
  }
  if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) {
    m_camera.m_position -= m_camera.right() * speed;
  }
  if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) {
    m_camera.m_position += m_camera.right() * speed;
  }

  m_camera.update_view_matrix();
}

void camera_controller::on_mouse_moved(double _xpos, double _ypos) {
  if (m_first_mouse) {
    m_first_mouse = false;
    m_last_xpos = _xpos;
    m_last_ypos = _ypos;
    return;
  }

  float xoffset = (_xpos - m_last_xpos) * sensitivity;
  float yoffset =
      (_ypos - m_last_ypos) *
      sensitivity; // Reversed since y-coordinates go from bottom to top
  m_last_xpos = _xpos;
  m_last_ypos = _ypos;

  // Get current camera vectors
  glm::vec3 front = glm::normalize(m_camera.m_front);
  glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);

  // Horizontal rotation (yaw): rotate around world up axis
  float yaw_rad = glm::radians(-xoffset); // Negative for correct direction
  glm::mat4 yaw_rotation = glm::rotate(glm::mat4(1.0f), yaw_rad, world_up);
  front = glm::vec3(yaw_rotation * glm::vec4(front, 0.0f));

  // Calculate right vector after yaw rotation (needed for pitch rotation)
  glm::vec3 right = glm::normalize(glm::cross(front, world_up));

  // Vertical rotation (pitch): rotate around right axis
  float pitch_rad = glm::radians(-yoffset); // Negative for correct direction
  glm::mat4 pitch_rotation = glm::rotate(glm::mat4(1.0f), pitch_rad, right);
  glm::vec3 new_front = glm::vec3(pitch_rotation * glm::vec4(front, 0.0f));

  // Clamp pitch: check angle between new_front and world_up
  // Angle should be between (90 - max_pitch) and (90 + max_pitch) degrees
  // which means pitch is between -max_pitch and +max_pitch
  float angle_with_up =
      glm::degrees(glm::acos(glm::dot(glm::normalize(new_front), world_up)));
  float min_angle = 90.0f - max_pitch_angle;
  float max_angle = 90.0f + max_pitch_angle;

  if (angle_with_up >= min_angle && angle_with_up <= max_angle) {
    front = new_front;
  }
  // Otherwise, ignore the pitch change (front stays at current value after yaw)

  m_camera.m_front = glm::normalize(front);
  m_camera.update_view_matrix();
}

void camera_controller::on_mouse_scroll(double _xoffset, double _yoffset) {
  // Adjust FOV based on scroll offset (yoffset is the main scroll direction)
  m_camera.m_fov -= static_cast<float>(_yoffset) * scroll_sensitivity;
  m_camera.m_fov = glm::clamp(m_camera.m_fov, min_fov, max_fov);
  m_camera.update_projection_matrix();
}