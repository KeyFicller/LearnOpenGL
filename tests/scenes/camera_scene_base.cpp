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

void camera_scene_base::render_camera_ui() {
  ImGui::Checkbox("Camera Controller", &m_camera_controller_enabled);
}

