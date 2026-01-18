#include "camera_scene_base.h"

#include "imgui.h"

camera_scene_base::camera_scene_base(const std::string &_name)
    : test_scene_base(_name) {}

camera_scene_base::~camera_scene_base() {
  if (m_camera_controller) {
    delete m_camera_controller;
    m_camera_controller = nullptr;
  }
}

void camera_scene_base::init(GLFWwindow *_window) {
  test_scene_base::init(_window);
  m_camera_controller = new camera_controller(m_camera, _window);
}

void camera_scene_base::update(float _delta_time) {
  if (m_camera_controller_enabled && m_camera_controller) {
    m_camera_controller->update(_delta_time);
  }
}

bool camera_scene_base::on_mouse_moved(double _xpos, double _ypos) {
  if (m_camera_controller_enabled && m_camera_controller) {
    m_camera_controller->on_mouse_moved(_xpos, _ypos);
  }
  return false;
}

bool camera_scene_base::on_mouse_scroll(double _xoffset, double _yoffset) {
  if (m_camera_controller_enabled && m_camera_controller) {
    m_camera_controller->on_mouse_scroll(_xoffset, _yoffset);
  }
  return false;
}

void camera_scene_base::on_framebuffer_resized(int _width, int _height) {
  if (m_camera_controller) {
    m_camera_controller->on_framebuffer_resized(_width, _height);
  }
}

void camera_scene_base::render_camera_ui() {
  ImGui::Checkbox("Camera Controller", &m_camera_controller_enabled);

  if (m_camera_controller_enabled && m_camera_controller) {
    ImGui::Indent();

    if (ImGui::Checkbox("Orthographic", &m_camera.m_orthographic)) {
      m_camera.update_projection_matrix();
    }

    // Mouse capture toggle
    bool mouse_captured = m_camera_controller->is_mouse_captured();
    if (ImGui::Checkbox("Capture Mouse", &mouse_captured)) {
      m_camera_controller->set_mouse_captured(mouse_captured);
    }

    // Configuration
    if (ImGui::CollapsingHeader("Camera Settings")) {
      float speed = m_camera_controller->get_movement_speed();
      if (ImGui::SliderFloat("Movement Speed", &speed, 0.1f, 10.0f, "%.1f")) {
        m_camera_controller->set_movement_speed(speed);
      }

      float sensitivity = m_camera_controller->get_mouse_sensitivity();
      if (ImGui::SliderFloat("Mouse Sensitivity", &sensitivity, 0.01f, 1.0f,
                             "%.2f")) {
        m_camera_controller->set_mouse_sensitivity(sensitivity);
      }

      float scroll_sensitivity = m_camera_controller->get_scroll_sensitivity();
      if (ImGui::SliderFloat("Scroll Sensitivity", &scroll_sensitivity, 0.1f,
                             10.0f, "%.1f")) {
        m_camera_controller->set_scroll_sensitivity(scroll_sensitivity);
      }
    }

    ImGui::Unindent();
  }
}
