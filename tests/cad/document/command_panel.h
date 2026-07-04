#pragma once

#include "tests/cad/document/ui_icons.h"

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
                       command_factory factory, ui_icon_id icon);

  /** Clear all registered buttons (e.g., for plugin reload). */
  void clear_buttons();

  /** Push a contextual button set (e.g., sketch mode). */
  void push_context(const char *name);
  void pop_context();

  /** Register a button in the current context (call after push_context). */
  void register_context_button(const char *label, const char *tooltip,
                               command_factory factory, ui_icon_id icon);

private:
  command_panel() = default;
  ~command_panel() = default;

  struct button_entry {
    const char *label;
    const char *tooltip;
    command_factory factory;
    ui_icon_id icon;
  };

  struct context_entry {
    const char *name = nullptr;
    std::vector<button_entry> buttons;
  };

  std::vector<button_entry> m_buttons;
  std::vector<context_entry> m_context_stack;
};

} // namespace toy_cad::interaction
