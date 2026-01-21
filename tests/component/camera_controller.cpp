#include "tests/component/camera_controller.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

camera_controller::camera_controller(camera &_camera, GLFWwindow *window)
    : m_camera(_camera), m_window(window) {}

void camera_controller::update(float _delta_time) {
  if (!m_mouse_captured) {
    return;
  }

  float speed = m_movement_speed * _delta_time;

  // W/S: Move forward/backward
  if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) {
    m_camera.m_position += m_camera.front() * speed;
  }
  if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) {
    m_camera.m_position -= m_camera.front() * speed;
  }

  // A/D: Move left/right
  if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) {
    m_camera.m_position -= m_camera.right() * speed;
  }
  if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) {
    m_camera.m_position += m_camera.right() * speed;
  }

  // Q/E or Space/Shift: Move up/down
  if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    m_camera.m_position += m_camera.m_world_up * speed;
  }
  if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
    m_camera.m_position -= m_camera.m_world_up * speed;
  }

  m_camera.update_view_matrix();
}

void camera_controller::on_mouse_moved(double _xpos, double _ypos) {
  if (!m_mouse_captured) {
    m_first_mouse = true;
    return;
  }

  if (m_first_mouse) {
    m_first_mouse = false;
    m_last_xpos = _xpos;
    m_last_ypos = _ypos;
    return;
  }

  float xoffset = static_cast<float>(_xpos - m_last_xpos) * m_mouse_sensitivity;
  float yoffset =
      static_cast<float>(m_last_ypos - _ypos) *
      m_mouse_sensitivity; // Reversed since y-coordinates go from bottom to top
  m_last_xpos = _xpos;
  m_last_ypos = _ypos;

  // Update yaw and pitch
  m_camera.m_yaw += xoffset;
  m_camera.m_pitch += yoffset;

  // Clamp pitch to prevent gimbal lock
  if (m_camera.m_pitch > m_max_pitch) {
    m_camera.m_pitch = m_max_pitch;
  }
  if (m_camera.m_pitch < -m_max_pitch) {
    m_camera.m_pitch = -m_max_pitch;
  }

  m_camera.update_view_matrix();
}

void camera_controller::on_mouse_scroll(double _xoffset, double _yoffset) {
  // Adjust FOV based on scroll offset (yoffset is the main scroll direction)
  m_camera.m_fov -= static_cast<float>(_yoffset) * m_scroll_sensitivity;
  if (m_camera.m_fov < m_min_fov) {
    m_camera.m_fov = m_min_fov;
  }
  if (m_camera.m_fov > m_max_fov) {
    m_camera.m_fov = m_max_fov;
  }
  m_camera.update_projection_matrix();
}

void camera_controller::on_framebuffer_resized(int _width, int _height) {
  if (_height > 0) {
    m_camera.set_aspect_ratio(static_cast<float>(_width) /
                              static_cast<float>(_height));
  }
}

void camera_controller::set_mouse_captured(bool _captured) {
  m_mouse_captured = _captured;
  // if (_captured) {
  //   glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  //   m_first_mouse = true;
  // } else {
  //   glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  // }
}
