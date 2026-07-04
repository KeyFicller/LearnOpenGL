#include "viewport_toolbar.h"

#include "tests/cad/document/ui_icons.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/selection_manager.h"
#include "tests/cad/renderer/display_mode.h"

#include "imgui.h"

namespace toy_cad::interaction {

viewport_toolbar &viewport_toolbar::instance() {
  static viewport_toolbar s;
  return s;
}

void viewport_toolbar::draw_in_viewport(std::function<void()> fit_view) {
  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  if (!draw_list) {
    return;
  }

  auto &inst = instance::get();
  auto &icons = ui_icons::instance();
  const bool solid_active = inst.display_mode() == mesh_display_mode::solid;
  const bool wire_active = inst.display_mode() == mesh_display_mode::wireframe;

  constexpr float k_margin = 10.0f;
  constexpr float k_pad = 6.0f;
  constexpr float k_btn_size = 32.0f;
  constexpr float k_gap = 4.0f;
  constexpr int k_btn_count = 4;

  const ImVec2 win_pos = ImGui::GetWindowPos();
  const ImVec2 content_min = ImGui::GetWindowContentRegionMin();
  const ImVec2 content_max = ImGui::GetWindowContentRegionMax();
  const float region_w = content_max.x - content_min.x;
  const float toolbar_w =
      k_pad * 2.0f + k_btn_size * static_cast<float>(k_btn_count) +
      k_gap * static_cast<float>(k_btn_count - 1);
  const float toolbar_h = k_pad * 2.0f + k_btn_size;

  const ImVec2 origin(win_pos.x + content_min.x, win_pos.y + content_min.y);
  const ImVec2 toolbar_min(origin.x + region_w - toolbar_w - k_margin,
                           origin.y + k_margin);
  const ImVec2 toolbar_max(toolbar_min.x + toolbar_w, toolbar_min.y + toolbar_h);

  draw_list->AddRectFilled(toolbar_min, toolbar_max,
                           IM_COL32(20, 22, 28, 180), 8.0f);

  const ImVec2 btn_size(k_btn_size, k_btn_size);
  auto next_btn_min = [&](int index) {
    return ImVec2(toolbar_min.x + k_pad +
                      static_cast<float>(index) * (k_btn_size + k_gap),
                  toolbar_min.y + k_pad);
  };

  ImGui::SetCursorScreenPos(next_btn_min(0));
  if (icons.icon_button(ui_icon_id::fit_view, btn_size,
                        "Frame all model geometry in view", false,
                        icon_button_style::toolbar) &&
      fit_view) {
    fit_view();
  }

  ImGui::SetCursorScreenPos(next_btn_min(1));
  if (icons.icon_button(ui_icon_id::solid, btn_size, "Shaded solid display",
                        solid_active, icon_button_style::toolbar)) {
    inst.set_display_mode(mesh_display_mode::solid);
  }

  ImGui::SetCursorScreenPos(next_btn_min(2));
  if (icons.icon_button(ui_icon_id::wireframe, btn_size, "Wireframe display",
                        wire_active, icon_button_style::toolbar)) {
    inst.set_display_mode(mesh_display_mode::wireframe);
  }

  ImGui::SetCursorScreenPos(next_btn_min(3));
  if (icons.icon_button(ui_icon_id::clear_selection, btn_size,
                        "Clear all subshape selections", false,
                        icon_button_style::toolbar)) {
    selection_manager::instance().clear_selection();
    std::printf("[viewport_toolbar] Cleared all selections\n");
  }
}

} // namespace toy_cad::interaction
