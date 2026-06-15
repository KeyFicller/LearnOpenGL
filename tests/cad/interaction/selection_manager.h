#pragma once

#include "tests/cad/geometry/subshape_handle.h"
#include "tests/cad/database/handle.h"

#include <glm/glm.hpp>
#include <optional>
#include <unordered_set>
#include <vector>

namespace toy_cad {
class box_feature;
}

namespace toy_cad::interaction {

/**
 * Combined subshape reference: feature handle + subshape handle.
 * Used to uniquely identify a face or edge within the scene.
 */
struct subshape_ref {
  handle feature_handle;  // Handle to the box_feature (or other primitive)
  geometry::subshape_handle subshape;

  [[nodiscard]] bool valid() const {
    return feature_handle.valid() && subshape.valid();
  }

  friend bool operator==(const subshape_ref &a, const subshape_ref &b) {
    return a.feature_handle == b.feature_handle &&
           a.subshape.id() == b.subshape.id();
  }
  friend bool operator!=(const subshape_ref &a, const subshape_ref &b) {
    return !(a == b);
  }
};

/**
 * Hash function for subshape_ref.
 */
struct subshape_ref_hash {
  [[nodiscard]] std::size_t operator()(const subshape_ref &ref) const noexcept {
    // Combine feature handle hash with subshape id hash
    std::size_t h1 = handle_hash{}(ref.feature_handle);
    std::size_t h2 = geometry::subshape_handle_hash{}(ref.subshape);
    return h1 ^ (h2 << 1);
  }
};

/**
 * Box selection mode determined by drag direction.
 * - window: top-left → bottom-right, object must be fully inside the rect
 * - crossing: bottom-right → top-left (or any reverse drag), object intersects the rect
 */
enum class box_select_mode : uint8_t {
  window = 0,
  crossing = 1,
};

/**
 * Selection state for a subshape (hover, selected, etc.)
 */
enum class selection_state : uint8_t {
  none = 0,
  hover = 1,      // Currently under mouse cursor
  selected = 2,   // Explicitly selected by user
  preview = 3,    // Preview of what would be selected (e.g., during drag)
};

/**
 * Color constants for selection visualization.
 */
struct selection_colors {
  static constexpr glm::vec3 hover = {1.0f, 1.0f, 0.0f};      // Yellow
  static constexpr glm::vec3 selected = {0.0f, 1.0f, 0.0f};     // Green
  static constexpr glm::vec3 preview = {0.5f, 1.0f, 0.5f};    // Light green
  static constexpr glm::vec3 edge_highlight = {1.0f, 0.5f, 0.0f}; // Orange for edges
};

/**
 * Singleton manager for CAD object selection state.
 * Manages hover, click selection, and box selection.
 */
class selection_manager {
public:
  static selection_manager &instance();

  selection_manager(const selection_manager &) = delete;
  selection_manager &operator=(const selection_manager &) = delete;
  selection_manager(selection_manager &&) noexcept = default;
  selection_manager &operator=(selection_manager &&) noexcept = default;

  /** Clear all selection state. */
  void clear();

  /** Clear hover state only. */
  void clear_hover();

  /** Clear selection set only. */
  void clear_selection();

  /** Get current hover target (if any). */
  [[nodiscard]] std::optional<subshape_ref> hover_target() const {
    return m_hover_target;
  }

  /**
   * Update hover state based on ray intersection.
   * Tests ray against all features in the scene.
   * @param ray_origin Ray origin in world space
   * @param ray_dir Ray direction (normalized)
   * @return true if hover target changed, false otherwise
   */
  bool update_hover(const glm::vec3 &ray_origin, const glm::vec3 &ray_dir);

  /**
   * Manually set hover target (e.g., from UI).
   */
  void set_hover_target(const subshape_ref &target);

  /**
   * Clear hover target.
   */
  void clear_hover_target();

  /** Check if a subshape is currently hovered. */
  [[nodiscard]] bool is_hovered(const subshape_ref &ref) const;

  /** Check if a subshape is selected. */
  [[nodiscard]] bool is_selected(const subshape_ref &ref) const;

  /**
   * Toggle selection of a subshape.
   * @param ref The subshape to toggle
   * @param multi If true, adds to selection; if false, replaces selection
   * @return true if selection state changed
   */
  bool toggle_select(const subshape_ref &ref, bool multi = false);

  /**
   * Add a subshape to selection.
   */
  void select(const subshape_ref &ref);

  /**
   * Remove a subshape from selection.
   */
  void deselect(const subshape_ref &ref);

  /**
   * Replace current selection with a single item.
   */
  void select_single(const subshape_ref &ref);

  /**
   * Perform box selection using a screen rectangle.
   * @param screen_min Top-left corner of selection rect (pixels)
   * @param screen_max Bottom-right corner of selection rect (pixels)
   * @param viewport_size Total viewport dimensions (pixels)
   * @param clip_from_world Clip-space transformation matrix
   * @param multi If true, adds to selection; if false, replaces selection
   * @return Number of items selected
   */
  size_t box_select(const glm::vec2 &screen_min,
                    const glm::vec2 &screen_max,
                    const glm::vec2 &viewport_size,
                    const glm::mat4 &clip_from_world,
                    bool multi = false);

  /**
   * Get all currently selected subshapes.
   */
  [[nodiscard]] const std::unordered_set<subshape_ref, subshape_ref_hash> &
  selection() const {
    return m_selection;
  }

  /**
   * Get all selected subshapes as a vector (for iteration).
   */
  void get_selection_vector(std::vector<subshape_ref> &out) const;

  /**
   * Begin a drag selection operation.
   */
  void begin_drag_select(const glm::vec2 &start_pos);

  /**
   * Update drag selection (while dragging).
   * @return true if preview selection changed
   */
  bool update_drag_select(const glm::vec2 &current_pos,
                          const glm::vec2 &viewport_size,
                          const glm::mat4 &clip_from_world);

  /**
   * End drag selection and apply to actual selection.
   * @param multi If true, adds to selection; if false, replaces
   */
  void end_drag_select(bool multi);

  /**
   * Cancel current drag selection.
   */
  void cancel_drag_select();

  /**
   * Check if currently in drag selection mode.
   */
  [[nodiscard]] bool is_drag_selecting() const { return m_drag_active; }

  /** Preview subshapes while box-selecting. */
  [[nodiscard]] const std::vector<subshape_ref> &drag_preview() const {
    return m_drag_preview;
  }

  /** Current box-select mode from drag direction (only meaningful while dragging). */
  [[nodiscard]] box_select_mode drag_select_mode() const {
    if (!m_drag_active) {
      return box_select_mode::window;
    }
    return (m_drag_current.x >= m_drag_start.x && m_drag_current.y >= m_drag_start.y)
               ? box_select_mode::window
               : box_select_mode::crossing;
  }

  /**
   * Get current drag selection rectangle (if active).
   */
  [[nodiscard]] std::optional<std::pair<glm::vec2, glm::vec2>> drag_rect() const;

private:
  selection_manager() = default;
  ~selection_manager() = default;

  // Current hover target (under mouse cursor)
  std::optional<subshape_ref> m_hover_target;

  // Set of selected subshapes
  std::unordered_set<subshape_ref, subshape_ref_hash> m_selection;

  // Drag selection state
  bool m_drag_active = false;
  glm::vec2 m_drag_start{0.0f};
  glm::vec2 m_drag_current{0.0f};

  // Preview selection during drag
  std::vector<subshape_ref> m_drag_preview;

  /**
   * Find the closest intersected subshape along a ray.
   * @return Optional reference to the hit subshape
   */
  [[nodiscard]] std::optional<subshape_ref> ray_cast(
      const glm::vec3 &ray_origin, const glm::vec3 &ray_dir) const;

  /**
   * Find all subshapes matching a screen-space selection rectangle.
   */
  void screen_rect_select(const glm::vec2 &screen_min,
                          const glm::vec2 &screen_max,
                          const glm::vec2 &viewport_size,
                          const glm::mat4 &clip_from_world,
                          box_select_mode mode,
                          std::vector<subshape_ref> &out_hits) const;

  /**
   * Find all subshapes within a view frustum (legacy crossing helper).
   */
  void frustum_select(const glm::vec4 frustum_planes[6],
                      std::vector<subshape_ref> &out_hits) const;

  /**
   * Notify that selection changed (for UI updates).
   */
  void on_selection_changed();
};

} // namespace toy_cad::interaction
