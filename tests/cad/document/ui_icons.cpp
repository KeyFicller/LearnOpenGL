#include "ui_icons.h"

#include "basic/texture.h"

#include "imgui.h"

namespace toy_cad::interaction {

namespace {

struct icon_spec {
  ui_icon_id id;
  const char *path;
};

constexpr icon_spec k_icon_specs[] = {
    {ui_icon_id::box, "assets/images/ui/box.png"},
    {ui_icon_id::extrude, "assets/images/ui/extrude.png"},
    {ui_icon_id::fit_view, "assets/images/ui/fit_view.png"},
    {ui_icon_id::solid, "assets/images/ui/solid.png"},
    {ui_icon_id::wireframe, "assets/images/ui/wireframe.png"},
    {ui_icon_id::clear_selection, "assets/images/ui/clear_selection.png"},
};

} // namespace

ui_icons &ui_icons::instance() {
  static ui_icons s;
  return s;
}

const ui_icons::icon_entry *ui_icons::find(ui_icon_id id) const {
  for (const auto &entry : m_icons) {
    if (entry.id == id) {
      return &entry;
    }
  }
  return nullptr;
}

void ui_icons::init() {
  if (m_initialized) {
    return;
  }

  m_icons.clear();
  for (const auto &spec : k_icon_specs) {
    icon_entry entry{spec.id, spec.path, nullptr};
    try {
      entry.texture = std::make_unique<texture_2d>(
          spec.path, wrap_mode::k_clamp_to_edge, filter_mode::k_linear);
    } catch (const std::exception &) {
      entry.texture.reset();
    }
    m_icons.push_back(std::move(entry));
  }
  m_initialized = true;
}

void ui_icons::shutdown() {
  m_icons.clear();
  m_initialized = false;
}

bool ui_icons::loaded(ui_icon_id id) const {
  const auto *entry = find(id);
  return entry && entry->texture;
}

ImTextureID ui_icons::texture(ui_icon_id id) const {
  const auto *entry = find(id);
  if (!entry || !entry->texture) {
    return ImTextureID{};
  }
  return (ImTextureID)(intptr_t)entry->texture->ID();
}

bool ui_icons::icon_button(ui_icon_id id, const ImVec2 &size, const char *tooltip,
                           bool active, icon_button_style style) {
  const auto *entry = find(id);
  if (!entry || !entry->texture) {
    return false;
  }

  const bool toolbar = style == icon_button_style::toolbar;
  const float frame_padding = toolbar ? 5.0f : 4.0f;
  const float frame_rounding = toolbar ? 6.0f : 4.0f;

  ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                      ImVec2(frame_padding, frame_padding));
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, frame_rounding);

  if (active) {
    ImGui::PushStyleColor(
        ImGuiCol_Button,
        toolbar ? ImVec4(0.35f, 0.55f, 0.85f, 0.55f)
                : ImVec4(0.28f, 0.48f, 0.78f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        toolbar ? ImVec4(0.40f, 0.60f, 0.90f, 0.65f)
                : ImVec4(0.32f, 0.52f, 0.82f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        toolbar ? ImVec4(0.30f, 0.50f, 0.80f, 0.75f)
                : ImVec4(0.24f, 0.44f, 0.72f, 1.0f));
  } else if (toolbar) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(1.0f, 1.0f, 1.0f, 0.10f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(1.0f, 1.0f, 1.0f, 0.16f));
  }

  const ImVec4 tint(1.0f, 1.0f, 1.0f, 1.0f);
  const bool clicked = ImGui::ImageButton(
      entry->path, texture(id), size, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f),
      ImVec4(0.0f, 0.0f, 0.0f, 0.0f), tint);

  if (active || toolbar) {
    ImGui::PopStyleColor(3);
  }
  ImGui::PopStyleVar(3);

  if (tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::SetTooltip("%s", tooltip);
  }

  return clicked;
}

} // namespace toy_cad::interaction
