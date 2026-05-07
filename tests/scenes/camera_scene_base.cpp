#include "camera_scene_base.h"

#include "basic/imgui_form.h"

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
  if (ImGui::Checkbox("Camera Controller", &m_camera_controller_enabled))
    m_camera_controller->set_mouse_captured(m_camera_controller_enabled);

  if (m_camera_controller_enabled && m_camera_controller) {
    ImGui::Indent();

    ImGui::TextDisabled("Ctrl+drag: pan · Alt+drag: orbit");
    ImGui::Spacing();

    int projection = m_camera.Orthographic ? 1 : 0;
    ImGui::PushItemWidth(imgui_form_item_width(0.78f));
    if (ImGui::Combo("Projection", &projection,
                     "Perspective\0Orthographic\0")) {
      m_camera.Orthographic = (projection == 1);
      m_camera.set_aspect_ratio(m_camera.AspectRatio);
    }
    ImGui::PopItemWidth();

    // Configuration
    if (ImGui::CollapsingHeader("Camera Settings")) {
      float speed = m_camera_controller->get_movement_speed();
      if (ImGui::SliderFloat("Movement Speed", &speed, 0.1f, 10.0f, "%.1f")) {
        m_camera_controller->set_movement_speed(speed);
      }

      float pan = m_camera_controller->get_pan_sensitivity();
      if (ImGui::SliderFloat("Pan sensitivity", &pan, 0.001f, 0.03f, "%.4f")) {
        m_camera_controller->set_pan_sensitivity(pan);
      }

      float orb = m_camera_controller->get_orbit_sensitivity();
      if (ImGui::SliderFloat("Orbit sensitivity", &orb, 0.02f, 0.5f, "%.2f")) {
        m_camera_controller->set_orbit_sensitivity(orb);
      }

      float rad = m_camera_controller->get_orbit_radius();
      if (ImGui::SliderFloat("Orbit target distance", &rad, 0.5f, 30.0f,
                             "%.1f")) {
        m_camera_controller->set_orbit_radius(rad);
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Alt-orbit rotates around the point this far ahead on the view ray "
            "when you press Alt (re-armed each Alt+drag session).");
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
