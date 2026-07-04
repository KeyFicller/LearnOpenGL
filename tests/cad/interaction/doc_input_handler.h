#pragma once

#include "input_handler.h"
#include "tests/cad/database/handle.h"
#include "tests/cad/interaction/selection_manager.h"

#include <glm/glm.hpp>
#include <unordered_set>

namespace toy_cad::interaction {

/** Singleton: document-level selection and default input stack entry.
 *  Handles mouse hover, click selection, and box selection.
 */
class doc_input_handler final : public input_handler {
public:
  static doc_input_handler &instance();

  doc_input_handler(const doc_input_handler &) = delete;
  doc_input_handler &operator=(const doc_input_handler &) = delete;

  /** Legacy selection API - clears object-level selection */
  void clear_selection();

  [[nodiscard]] bool is_selected(handle h) const;

  /** Call from explorer tree on left click (Ctrl = toggle in set, else replace). */
  void on_explorer_item_left_click(handle item);

  [[nodiscard]] const std::unordered_set<handle, handle_hash> &selection()
      const noexcept {
    return m_selection;
  }

  /** Input handler overrides for 3D picking */
  bool on_mouse_moved(double xpos, double ypos) override;
  bool on_mouse_button(int button, int action, int mods) override;

  /**
   * Check if currently dragging for box selection.
   */
  [[nodiscard]] bool is_dragging() const { return m_is_dragging; }

  /**
   * Get current drag selection rectangle (if active).
   */
  [[nodiscard]] std::optional<std::pair<glm::vec2, glm::vec2>>
  drag_rect() const;

private:
  doc_input_handler() = default;

  // Legacy object-level selection (for tree explorer)
  std::unordered_set<handle, handle_hash> m_selection{};

  // Left-button press / drag for box selection (small motion = click pick)
  bool m_left_button_down = false;
  glm::vec2 m_press_pos{0.0f};
  bool m_is_dragging = false;
  glm::vec2 m_drag_start{0.0f};
  glm::vec2 m_drag_current{0.0f};

  // Track last mouse position for hover updates
  glm::vec2 m_last_mouse_pos{0.0f};

  static constexpr float k_box_drag_threshold_px = 4.0f;

  /**
   * Update hover state based on mouse position.
   */
  void update_hover(double xpos, double ypos);

  /**
   * Handle click selection.
   */
  void handle_click_selection(double xpos, double ypos, bool multi);

  /** Start box-selection drag after pointer moved past threshold. */
  void begin_drag_selection(const glm::vec2 &start_pos);

  /** Promote press+move into box drag when movement exceeds threshold. */
  void maybe_begin_drag_from_press(const glm::vec2 &current_pos);

  /**
   * Update drag selection.
   */
  void update_drag_selection(const glm::vec2 &current_pos);

  /**
   * End drag selection and apply.
   */
  void end_drag_selection(bool multi);

  /**
   * Cancel current drag.
   */
  void cancel_drag_selection();
};

} // namespace toy_cad::interaction
