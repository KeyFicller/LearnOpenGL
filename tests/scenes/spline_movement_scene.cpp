#include "spline_movement_scene.h"

#include "spline_movement_sub_scenes.h"

spline_movement_scene::spline_movement_scene()
    : renderable_scene_base("Spline Movement") {}

void spline_movement_scene::init(GLFWwindow *_window) {
  renderable_scene_base::init(_window);

  // Initialize camera
  m_camera.Position = {0.0f, 0.0f, 3.0f};
  m_camera.Yaw = -90.0f;
  m_camera.Pitch = 0.0f;
  m_camera.Left = -1.0f;
  m_camera.Right = 1.0f;
  m_camera.Bottom = -1.0f;
  m_camera.Top = 1.0f;
  m_camera.Orthographic = true;
  m_camera.update_view_matrix();

  m_sub_scene_manager.add_sub_scene(
      std::make_unique<spline_movement_snake_sub_scene>(this));
}

void spline_movement_scene::render() { m_sub_scene_manager.render(); }

void spline_movement_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Spline Movement");
  ImGui::Spacing();

  render_camera_ui();

  // Sub-scene selection UI
  ImGui::Separator();
  auto names = m_sub_scene_manager.get_names();
  int current_index = m_sub_scene_manager.current_index();
  if (ImGui::Combo("Scene Mode", &current_index, names.data(),
                   static_cast<int>(names.size()))) {
    m_sub_scene_manager.set_current(current_index);
  }

  // Render current sub-scene UI
  m_sub_scene_manager.render_ui();
}

void spline_movement_scene::update(float _delta_time) {
  m_sub_scene_manager.update(_delta_time);
}

bool spline_movement_scene::on_mouse_moved(double _xpos, double _ypos) {
  return m_sub_scene_manager.on_mouse_moved(_xpos, _ypos);
}