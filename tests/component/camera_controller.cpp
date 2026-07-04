#include "tests/component/camera_controller.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace {

void sync_yaw_pitch_from_direction(camera &_camera, const glm::vec3 &dir_unit) {
  _camera.Pitch = glm::degrees(std::asin(std::clamp(dir_unit.y, -1.0f, 1.0f)));
  _camera.Yaw = glm::degrees(std::atan2(dir_unit.z, dir_unit.x));
}

glm::vec3 safe_normalize(const glm::vec3 &v, const glm::vec3 &fallback) {
  const float len_sq = glm::dot(v, v);
  if (len_sq < 1e-12f) {
    return fallback;
  }
  return v / std::sqrt(len_sq);
}

} // namespace

// -----------------------------------------------------------------------------
camera_controller::camera_controller(camera &_camera, GLFWwindow *window)
    : m_camera(_camera), m_window(window) {
  if (m_window) {
    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(m_window, &w, &h);
    if (w > 0 && h > 0) {
      m_viewport_w = w;
      m_viewport_h = h;
    }
  }
}

int camera_controller::navigation_mod_mode() const {
  const bool ctrl = glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                    glfwGetKey(m_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
  const bool alt = glfwGetKey(m_window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
                   glfwGetKey(m_window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
  if (ctrl) {
    return 1;
  }
  if (alt) {
    return 2;
  }
  return 0;
}

void camera_controller::set_navigation_focus(const glm::vec3 &point) {
  const glm::vec3 forward = m_camera.front();
  const float dist = glm::dot(point - m_camera.Position, forward);
  m_dolly_distance = std::max(0.1f, dist);
  m_dolly_distance_valid = true;
  m_orbit_pivot = point;
  m_orbit_pivot_valid = true;
}

void camera_controller::clear_navigation_focus() {
  m_dolly_distance_valid = false;
  m_orbit_pivot_valid = false;
}

glm::vec3 camera_controller::navigation_pivot() const {
  const glm::vec3 forward = m_camera.front();
  if (m_dolly_distance_valid) {
    return m_camera.Position + forward * m_dolly_distance;
  }
  return m_camera.Position + forward * m_orbit_radius;
}

void camera_controller::update(float _delta_time) {
  if (!m_mouse_captured) {
    return;
  }

  float speed = m_movement_speed * _delta_time;
  glm::vec3 delta{0.0f};

  if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) {
    delta += m_camera.front() * speed;
  }
  if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) {
    delta -= m_camera.front() * speed;
  }
  if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) {
    delta -= m_camera.right() * speed;
  }
  if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) {
    delta += m_camera.right() * speed;
  }
  if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    delta += m_camera.WorldUp * speed;
  }
  if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS ||
      glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
    delta -= m_camera.WorldUp * speed;
  }

  if (glm::dot(delta, delta) > 0.0f) {
    m_camera.Position += delta;
  }

  m_camera.update_view_matrix();
}

void camera_controller::on_mouse_moved(double _xpos, double _ypos) {
  if (!m_mouse_captured) {
    m_first_mouse = true;
    m_prev_nav_mode = -1;
    m_orbit_pivot_valid = false;
    return;
  }

  const int nav_mode = navigation_mod_mode();
  if (nav_mode != m_prev_nav_mode) {
    m_last_xpos = _xpos;
    m_last_ypos = _ypos;
    m_prev_nav_mode = nav_mode;
    if (nav_mode != 2) {
      m_orbit_pivot_valid = false;
    }
    m_first_mouse = false;
    return;
  }

  if (m_first_mouse) {
    m_first_mouse = false;
    m_last_xpos = _xpos;
    m_last_ypos = _ypos;
    return;
  }

  const float xrel = static_cast<float>(_xpos - m_last_xpos);
  const float yrel = static_cast<float>(m_last_ypos - _ypos);
  m_last_xpos = _xpos;
  m_last_ypos = _ypos;

  if (nav_mode == 1) {
    // Ctrl: pan in view plane (right / up). Dolly distance stays fixed so the
    // pivot moves with the camera and remains the zoom center on screen.
    if (m_camera.Orthographic) {
      constexpr float pan_sensitivity_ref = 0.006f;
      const float world_w = m_camera.Right - m_camera.Left;
      const float world_h = m_camera.Top - m_camera.Bottom;
      const float px_to_world_x =
          world_w / std::max(1.0f, static_cast<float>(m_viewport_w));
      const float px_to_world_y =
          world_h / std::max(1.0f, static_cast<float>(m_viewport_h));
      const float scale = m_pan_sensitivity / pan_sensitivity_ref;
      m_camera.Position += m_camera.right() * (xrel * px_to_world_x * scale) +
                           m_camera.up() * (yrel * px_to_world_y * scale);
    } else {
      const float depth =
          m_dolly_distance_valid
              ? m_dolly_distance
              : std::max(0.5f, glm::length(m_camera.Position));
      const float k = m_pan_sensitivity * depth;
      m_camera.Position +=
          m_camera.right() * (xrel * k) + m_camera.up() * (yrel * k);
    }
    m_camera.update_view_matrix();
    return;
  }

  if (nav_mode == 2) {
    if (!m_orbit_pivot_valid) {
      m_orbit_pivot = navigation_pivot();
      m_orbit_pivot_valid = true;
    }

    const float yaw_deg = xrel * m_orbit_sensitivity;
    const float pitch_deg = yrel * m_orbit_sensitivity;

    glm::vec3 offset = m_camera.Position - m_orbit_pivot;

    glm::mat4 rot_yaw =
        glm::rotate(glm::mat4(1.0f), glm::radians(-yaw_deg), m_camera.WorldUp);
    offset = glm::vec3(rot_yaw * glm::vec4(offset, 0.0f));

    glm::vec3 pitch_axis =
        glm::cross(m_camera.WorldUp, safe_normalize(offset, m_camera.front()));
    pitch_axis = safe_normalize(pitch_axis, m_camera.right());
    glm::mat4 rot_pitch =
        glm::rotate(glm::mat4(1.0f), glm::radians(-pitch_deg), pitch_axis);
    offset = glm::vec3(rot_pitch * glm::vec4(offset, 0.0f));

    m_camera.Position = m_orbit_pivot + offset;

    const glm::vec3 to_target =
        safe_normalize(m_orbit_pivot - m_camera.Position, -m_camera.front());
    sync_yaw_pitch_from_direction(m_camera, to_target);
    m_camera.update_view_matrix();

    if (m_dolly_distance_valid) {
      m_dolly_distance =
          std::max(0.1f, glm::dot(m_orbit_pivot - m_camera.Position,
                                  m_camera.front()));
    }
    return;
  }

  // No modifier: mouse does not change camera orientation (use Ctrl/Alt+drag).
  (void)xrel;
  (void)yrel;
  m_orbit_pivot_valid = false;
}

void camera_controller::on_mouse_scroll(double /*_xoffset*/, double _yoffset) {
  if (m_camera.Orthographic) {
    const float zoom =
        std::exp(-static_cast<float>(_yoffset) * 0.08f * m_scroll_sensitivity);
    const float cx = (m_camera.Left + m_camera.Right) * 0.5f;
    const float cy = (m_camera.Bottom + m_camera.Top) * 0.5f;
    float hw = (m_camera.Right - m_camera.Left) * 0.5f * zoom;
    float hh = (m_camera.Top - m_camera.Bottom) * 0.5f * zoom;
    constexpr float k_min_half = 1e-4f;
    constexpr float k_max_half = 1e6f;
    hw = std::clamp(hw, k_min_half, k_max_half);
    hh = std::clamp(hh, k_min_half, k_max_half);
    m_camera.Left = cx - hw;
    m_camera.Right = cx + hw;
    m_camera.Bottom = cy - hh;
    m_camera.Top = cy + hh;
  } else {
    const float zoom_factor =
        std::exp(-static_cast<float>(_yoffset) * 0.1f * m_scroll_sensitivity);

    const glm::vec3 forward = m_camera.front();
    const glm::vec3 pivot = navigation_pivot();

    constexpr float k_min_dist = 0.1f;
    constexpr float k_max_dist = 1000.0f;
    const float dist_to_pivot =
        std::max(k_min_dist, glm::dot(pivot - m_camera.Position, forward));
    const float new_dist =
        std::clamp(dist_to_pivot * zoom_factor, k_min_dist, k_max_dist);

    m_camera.Position = pivot - forward * new_dist;
    if (m_dolly_distance_valid) {
      m_dolly_distance = new_dist;
    }
    m_camera.update_view_matrix();
  }
  m_camera.update_projection_matrix();
}

void camera_controller::on_framebuffer_resized(int _width, int _height) {
  if (_width > 0 && _height > 0) {
    m_viewport_w = _width;
    m_viewport_h = _height;
    m_camera.set_aspect_ratio(static_cast<float>(_width) /
                              static_cast<float>(_height));
  }
}

void camera_controller::set_mouse_captured(bool _captured) {
  m_mouse_captured = _captured;
  m_first_mouse = true;
  m_prev_nav_mode = -1;
  m_orbit_pivot_valid = false;
}
