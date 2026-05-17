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
    // Show active command status
    auto &dispatcher = command_dispatcher::instance();
    if (dispatcher.has_active_command()) {
      ImGui::Text("Active: %s", dispatcher.active_command()->name());
      if (ImGui::Button("Cancel (Esc)", ImVec2(-1, 0))) {
        dispatcher.pop_command();
      }
      ImGui::Separator();
    }

    // Command buttons
    if (!m_buttons.empty()) {
      ImGui::SeparatorText("Commands");
      for (const auto &btn : m_buttons) {
        if (ImGui::Button(btn.label, ImVec2(-1, 0))) {
          btn.factory();
        }
        if (btn.tooltip && ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s", btn.tooltip);
        }
      }
    }

    // Show hint when no commands registered
    if (m_buttons.empty()) {
      ImGui::TextDisabled("No commands registered");
    }
  }
  ImGui::End();
}

void command_panel::register_button(const char *label, const char *tooltip,
                                    command_factory factory) {
  if (label && factory) {
    m_buttons.push_back({label, tooltip, std::move(factory)});
  }
}

void command_panel::clear_buttons() { m_buttons.clear(); }

} // namespace toy_cad::interaction
