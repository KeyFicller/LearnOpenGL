#pragma once

#include "input_handler.h"
#include "tests/cad/database/handle.h"

#include <unordered_set>

namespace toy_cad::interaction {

/** Singleton: document-level selection and default input stack entry. */
class doc_input_handler final : public input_handler {
public:
  static doc_input_handler &instance();

  doc_input_handler(const doc_input_handler &) = delete;
  doc_input_handler &operator=(const doc_input_handler &) = delete;

  void clear_selection();

  [[nodiscard]] bool is_selected(handle h) const;

  /** Call from explorer tree on left click (Ctrl = toggle in set, else replace). */
  void on_explorer_item_left_click(handle item);

  [[nodiscard]] const std::unordered_set<handle, handle_hash> &selection()
      const noexcept {
    return m_selection;
  }

private:
  doc_input_handler() = default;

  std::unordered_set<handle, handle_hash> m_selection{};
};

} // namespace toy_cad::interaction
