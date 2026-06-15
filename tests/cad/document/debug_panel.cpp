#include "tests/cad/document/debug_panel.h"

#include "imgui.h"
#include <cmath>

namespace toy_cad::interaction {

debug_panel &debug_panel::instance() {
  static debug_panel s;
  return s;
}

void debug_panel::draw_ui() {
  if (ImGui::Begin("CAD Debug Panel")) {
    ImGui::SeparatorText("Mouse");
    ImGui::Text("Screen: (%.1f, %.1f)", m_mouse_x, m_mouse_y);

    ImGui::SeparatorText("Ray Pick (Default Working Plane)");
    if (m_ray_pick_valid) {
      ImGui::Text("World: (%.3f, %.3f, %.3f)",
                  m_ray_pick_coord.x, m_ray_pick_coord.y, m_ray_pick_coord.z);
      if (ImGui::Button("Copy Coordinates")) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%.4f, %.4f, %.4f",
                      m_ray_pick_coord.x, m_ray_pick_coord.y, m_ray_pick_coord.z);
        ImGui::SetClipboardText(buf);
      }
    } else {
      ImGui::TextDisabled("No valid intersection (ray parallel to plane)");
    }
  }
  ImGui::End();
}

void debug_panel::set_ray_pick_coordinate(const glm::vec3 &coord, bool valid) {
  m_ray_pick_coord = coord;
  m_ray_pick_valid = valid;
}

void debug_panel::set_mouse_position(double x, double y) {
  m_mouse_x = x;
  m_mouse_y = y;
}

} // namespace toy_cad::interaction
