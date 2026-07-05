#include "tests/cad/document/command_panel.h"

#include "tests/cad/interaction/command_dispatcher.h"
#include "imgui.h"

#include <string>
#include <utility>

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

    const context_entry *ctx =
        m_context_stack.empty() ? nullptr : &m_context_stack.back();
    if (ctx && !ctx->name.empty()) {
      ImGui::TextDisabled("Mode: %s", ctx->name.c_str());
    }

    // Snapshot buttons so callbacks (e.g. Exit Sketch) can pop context safely.
    const std::vector<button_entry> buttons =
        (ctx && !ctx->buttons.empty()) ? ctx->buttons : m_buttons;

    if (!buttons.empty()) {
      auto &icons = ui_icons::instance();
      constexpr float k_icon_size = 32.0f;
      constexpr float k_gap = 6.0f;

      for (std::size_t i = 0; i < buttons.size(); ++i) {
        const auto &btn = buttons[i];
        if (i > 0) {
          ImGui::SameLine(0.0f, k_gap);
        }

        const char *tooltip =
            btn.tooltip.empty() ? nullptr : btn.tooltip.c_str();
        const bool has_icon = icons.loaded(btn.icon);
        if (has_icon) {
          if (icons.icon_button(btn.icon, ImVec2(k_icon_size, k_icon_size),
                                tooltip)) {
            btn.factory();
          }
        } else if (ImGui::Button(btn.label.c_str(),
                                 ImVec2(k_icon_size, k_icon_size))) {
          btn.factory();
        }

        if (tooltip && ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s", tooltip);
        }
      }
    }

    if (buttons.empty()) {
      ImGui::TextDisabled("No commands registered");
    }
  }
  ImGui::End();
}

void command_panel::register_button(const char *label, const char *tooltip,
                                    command_factory factory, ui_icon_id icon) {
  if (label && factory) {
    m_buttons.push_back({label, tooltip ? tooltip : "", std::move(factory), icon});
  }
}

void command_panel::clear_buttons() { m_buttons.clear(); }

void command_panel::push_context(const char *name) {
  m_context_stack.push_back({name ? name : "", {}});
}

void command_panel::pop_context() {
  if (!m_context_stack.empty()) {
    m_context_stack.pop_back();
  }
}

void command_panel::register_context_button(const char *label,
                                            const char *tooltip,
                                            command_factory factory,
                                            ui_icon_id icon) {
  if (m_context_stack.empty() || !label || !factory) {
    return;
  }
  m_context_stack.back().buttons.push_back(
      {label, tooltip ? tooltip : "", std::move(factory), icon});
}

} // namespace toy_cad::interaction
