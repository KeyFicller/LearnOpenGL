#pragma once

#include "tests/cad/database/handle.h"

namespace toy_cad::interaction {

/** Side panel: resolves a DB handle and calls `object::inspect()`. */
class inspector final {
public:
  static inspector &instance();

  inspector(const inspector &) = delete;
  inspector &operator=(const inspector &) = delete;

  void set_target(handle h);
  void clear_target();

  [[nodiscard]] handle target() const noexcept { return m_target; }

  void draw_ui();

private:
  inspector() = default;

  handle m_target{};
};

/** Call inside `tree_item_context_menu` when row `h` refers to this node. */
void append_tree_inspector_menu_items(handle h);

} // namespace toy_cad::interaction
