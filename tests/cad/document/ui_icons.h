#pragma once

#include "imgui.h"

#include <memory>
#include <vector>

class texture_2d;

namespace toy_cad::interaction {

enum class ui_icon_id {
  box,
  extrude,
  fit_view,
  solid,
  wireframe,
  clear_selection,
};

enum class icon_button_style {
  panel,
  toolbar,
};

/**
 * Loads toolbar / command panel icons from assets/images/ui.
 */
class ui_icons {
public:
  static ui_icons &instance();

  ui_icons(const ui_icons &) = delete;
  ui_icons &operator=(const ui_icons &) = delete;
  ui_icons(ui_icons &&) noexcept = default;
  ui_icons &operator=(ui_icons &&) noexcept = default;

  void init();
  void shutdown();

  bool loaded(ui_icon_id id) const;
  ImTextureID texture(ui_icon_id id) const;

  /** Icon button with optional active highlight. Returns true when clicked. */
  bool icon_button(ui_icon_id id, const ImVec2 &size, const char *tooltip = nullptr,
                   bool active = false,
                   icon_button_style style = icon_button_style::panel);

private:
  ui_icons() = default;
  ~ui_icons() = default;

  struct icon_entry {
    ui_icon_id id;
    const char *path;
    std::unique_ptr<texture_2d> texture;
  };

  const icon_entry *find(ui_icon_id id) const;

  std::vector<icon_entry> m_icons;
  bool m_initialized = false;
};

} // namespace toy_cad::interaction
