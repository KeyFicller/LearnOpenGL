#include "tests/cad/document/command_panel.h"

#include "tests/cad/interaction/command_dispatcher.h"
#include "imgui.h"

namespace toy_cad::interaction {

command_panel &command_panel::instance() {
  static command_panel s;
  return s;
}

void command_panel::draw_ui() {
  if (ImGui::Begin("Command Panel")) {
    auto &dispatcher = command_dispatcher::instance();
    if (dispatcher.has_active_command()) {
      ImGui::Text("Active: %s", dispatcher.active_command()->name());
      if (ImGui::Button("Cancel (Esc)", ImVec2(-1, 0))) {
        dispatcher.pop_command();
      }
      ImGui::Separator();
    }

    if (!m_buttons.empty()) {
      auto &icons = ui_icons::instance();
      constexpr float k_icon_size = 32.0f;
      constexpr float k_gap = 6.0f;

      for (std::size_t i = 0; i < m_buttons.size(); ++i) {
        const auto &btn = m_buttons[i];
        if (i > 0) {
          ImGui::SameLine(0.0f, k_gap);
        }

        const bool has_icon = icons.loaded(btn.icon);
        if (has_icon) {
          if (icons.icon_button(btn.icon, ImVec2(k_icon_size, k_icon_size),
                                btn.tooltip)) {
            btn.factory();
          }
        } else if (ImGui::Button(btn.label, ImVec2(k_icon_size, k_icon_size))) {
          btn.factory();
          if (btn.tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", btn.tooltip);
          }
        } else if (btn.tooltip && ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s", btn.tooltip);
        }
      }
    }

    if (m_buttons.empty()) {
      ImGui::TextDisabled("No commands registered");
    }
  }
  ImGui::End();
}

void command_panel::register_button(const char *label, const char *tooltip,
                                    command_factory factory, ui_icon_id icon) {
  if (label && factory) {
    m_buttons.push_back({label, tooltip, std::move(factory), icon});
  }
}

void command_panel::clear_buttons() { m_buttons.clear(); }

} // namespace toy_cad::interaction
