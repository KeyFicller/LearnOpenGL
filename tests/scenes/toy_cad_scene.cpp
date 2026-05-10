#include "toy_cad_scene.h"

#include "imgui.h"
#include "tests/cad/instance.h"

toy_cad_scene::toy_cad_scene() : renderable_scene_base("Toy CAD") {}

void toy_cad_scene::init(GLFWwindow *window) {
  renderable_scene_base::init(window);

  m_camera_controller_enabled = true;

  m_camera.Position = {0.0f, 0.0f, 3.0f};
  m_camera.Yaw = -90.0f;
  m_camera.Pitch = 0.0f;
  m_camera.update_view_matrix();

  toy_cad::instance::get().init(window);
}

void toy_cad_scene::update(float delta_time) {
  toy_cad::instance::get().update(delta_time);
  renderable_scene_base::update(delta_time);
  auto &disp = toy_cad::instance::get().disp();
  disp.camera_world_position = m_camera.Position;
  disp.view_matrix = m_camera.ViewMatrix;
  disp.projection_matrix = m_camera.ProjectionMatrix;
}

void toy_cad_scene::render() { toy_cad::instance::get().render(); }

void toy_cad_scene::render_ui() {
  toy_cad::instance::get().render_ui();

  ImGui::Separator();
  ImGui::Text("Toy CAD");
  ImGui::Spacing();

  render_camera_ui();
}

bool toy_cad_scene::on_mouse_moved(double xpos, double ypos) {
  if (toy_cad::instance::get().on_mouse_moved(xpos, ypos)) {
    return true;
  }
  return renderable_scene_base::on_mouse_moved(xpos, ypos);
}

bool toy_cad_scene::on_mouse_scroll(double xoffset, double yoffset) {
  if (toy_cad::instance::get().on_mouse_scroll(xoffset, yoffset)) {
    return true;
  }
  return renderable_scene_base::on_mouse_scroll(xoffset, yoffset);
}

bool toy_cad_scene::on_mouse_button(int button, int action, int mods) {
  if (toy_cad::instance::get().on_mouse_button(button, action, mods)) {
    return true;
  }
  return renderable_scene_base::on_mouse_button(button, action, mods);
}

void toy_cad_scene::on_framebuffer_resized(int width, int height) {
  toy_cad::instance::get().on_framebuffer_resized(width, height);
  renderable_scene_base::on_framebuffer_resized(width, height);
}

void toy_cad_scene::on_object_hovered(int object_id) {
  toy_cad::instance::get().on_object_hovered(object_id);
}
