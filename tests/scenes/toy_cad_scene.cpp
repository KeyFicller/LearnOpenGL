#include "toy_cad_scene.h"

#include "basic/camera.h"
#include "glad/gl.h"
#include "imgui.h"
#include "tests/cad/instance.h"
#include "tests/cad/renderer/highlight_renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace {

glm::mat4 projection_for_render_target(const camera &cam, int rw, int rh) {
  if (rw <= 0 || rh <= 0) {
    return cam.ProjectionMatrix;
  }
  const float aspect = static_cast<float>(rw) / static_cast<float>(rh);
  if (!cam.Orthographic) {
    return glm::perspective(glm::radians(cam.FOV), aspect, cam.Near, cam.Far);
  }
  const float cx = (cam.Left + cam.Right) * 0.5f;
  const float cy = (cam.Bottom + cam.Top) * 0.5f;
  const float hh = (cam.Top - cam.Bottom) * 0.5f;
  if (hh <= 1e-8f) {
    return cam.ProjectionMatrix;
  }
  const float hw = hh * aspect;
  return glm::ortho(cx - hw, cx + hw, cy - hh, cy + hh, cam.Near, cam.Far);
}

} // namespace

toy_cad_scene::toy_cad_scene() : renderable_scene_base("Toy CAD") {}

void toy_cad_scene::init(GLFWwindow *window) {
  renderable_scene_base::init(window);

  // CAD: Z up; on screen +Z up, +X toward lower-left, +Y toward lower-right.
  // Camera in XY plane on diagonal (√2 along X,Y), distance 3 from origin.
  constexpr float k_cam_r = 3.0f;
  const float s_xy = k_cam_r / std::sqrt(2.0f);
  m_camera.WorldUp = glm::vec3(0.f, 0.f, 1.f);
  m_camera.Position = glm::vec3(s_xy, s_xy, 0.f);
  m_camera.Yaw = 0.0f;
  m_camera.Pitch = -135.0f;
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
  disp.orthographic = m_camera.Orthographic;
}

void toy_cad_scene::render() {
  GLint vp[4]{};
  glGetIntegerv(GL_VIEWPORT, vp);
  auto &disp = toy_cad::instance::get().disp();
  disp.render_width = vp[2];
  disp.render_height = vp[3];
  disp.view_matrix = m_camera.ViewMatrix;
  disp.projection_matrix = projection_for_render_target(m_camera, vp[2], vp[3]);
  disp.orthographic = m_camera.Orthographic;

  toy_cad::instance::get().render();
}

void toy_cad_scene::render_ui() {
  toy_cad::instance::get().render_ui();

  ImGui::Separator();
  ImGui::Text("Toy CAD");
  ImGui::Spacing();

  render_camera_ui();
}

void toy_cad_scene::render_viewport_overlay() {
  toy_cad::renderer::highlight_renderer::instance().render_drag_rect_in_viewport();
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
