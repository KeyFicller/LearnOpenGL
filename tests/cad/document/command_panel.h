#pragma once

#include <functional>
#include <vector>

namespace toy_cad::interaction {

/**
 * Singleton: draws the command palette/button panel for CAD operations.
 *
 * Provides buttons to activate commands (create box, sketch, etc.).
 * Commands are actually dispatched via command_dispatcher.
 */
class command_panel {
public:
  static command_panel &instance();

  command_panel(const command_panel &) = delete;
  command_panel &operator=(const command_panel &) = delete;
  command_panel(command_panel &&) noexcept = default;
  command_panel &operator=(command_panel &&) noexcept = default;

  /** Draw the command panel UI (call inside ImGui context). */
  void draw_ui();

  /** Button callback type: creates and pushes a command to dispatcher. */
  using command_factory = std::function<void()>;

  /** Register a command button. Called during init to populate the panel. */
  void register_button(const char *label, const char *tooltip,
                       command_factory factory);

  /** Clear all registered buttons (e.g., for plugin reload). */
  void clear_buttons();

private:
  command_panel() = default;
  ~command_panel() = default;

  struct button_entry {
    const char *label;
    const char *tooltip;
    command_factory factory;
  };

  std::vector<button_entry> m_buttons;
};

} // namespace toy_cad::interaction
