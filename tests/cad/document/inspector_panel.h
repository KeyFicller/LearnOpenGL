#pragma once

#include "tests/cad/database/handle.h"

namespace toy_cad::interaction {

/** Side panel: resolves a DB handle and calls `object::inspect()`. */
class inspector_panel final {
public:
  static inspector_panel &instance();

  inspector_panel(const inspector_panel &) = delete;
  inspector_panel &operator=(const inspector_panel &) = delete;

  void set_target(handle h);
  void clear_target();

  [[nodiscard]] handle target() const noexcept { return m_target; }

  void draw_ui();

private:
  inspector_panel() = default;

  handle m_target{};
};

/** Call inside `tree_item_context_menu` when row `h` refers to this node. */
void append_tree_inspector_menu_items(handle h);

/** Backward compatibility: inspector = inspector_panel */
using inspector = inspector_panel;

} // namespace toy_cad::interaction
