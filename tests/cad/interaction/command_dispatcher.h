#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace toy_cad::interaction {

/**
 * Base class for CAD commands (e.g., create box, sketch, extrude).
 * Commands are managed by command_dispatcher and can be active/cancelled/completed.
 */
class command {
public:
  virtual ~command() = default;

  /** Command name for UI display. */
  [[nodiscard]] virtual const char *name() const = 0;

  /** Called when command becomes active. */
  virtual void on_activate() {}

  /** Called when command is cancelled (user pressed Escape). */
  virtual void on_cancel() {}

  /** Called when command completes successfully. */
  virtual void on_complete() {}

  /** Called each frame for UI updates. Return false to auto-complete. */
  virtual bool on_update() { return true; }

  /** Called for ImGui overlay/panel rendering during command. */
  virtual void on_draw_ui() {}

  /** Called for 3D scene rendering during command (e.g., preview geometry). */
  virtual void on_draw() {}

  /**
   * Called when an owned InputHandler reports input changed or confirmed.
   * @param flag Identifier for which input handler (e.g., "p1", "p2" for box command)
   * @param handler The InputHandler that triggered this call (extract data from it)
   * @param confirmed True if this is a confirmation (e.g., mouse click), false for preview
   */
  virtual void on_input_changed(const char *flag, class input_handler *handler, bool confirmed) {
    (void)flag;
    (void)handler;
    (void)confirmed;
  }
};

/**
 * Singleton: manages the active command stack and global command state.
 *
 * CAD workflows often involve nested commands (e.g., sketch -> create rectangle).
 * The dispatcher maintains a stack where only the top command receives input.
 */
class command_dispatcher {
public:
  static command_dispatcher &instance();

  command_dispatcher(const command_dispatcher &) = delete;
  command_dispatcher &operator=(const command_dispatcher &) = delete;
  command_dispatcher(command_dispatcher &&) noexcept = default;
  command_dispatcher &operator=(command_dispatcher &&) noexcept = default;

  /** Check if any command is currently active. */
  [[nodiscard]] bool has_active_command() const;

  /** Get the currently active command (top of stack), or nullptr. */
  [[nodiscard]] command *active_command() const;

  /** Push a new command onto the stack (becomes active). */
  void push_command(std::unique_ptr<command> cmd);

  /** Pop and destroy the top command (calls on_cancel if not completed). */
  void pop_command();

  /** Cancel all commands and clear the stack. */
  void cancel_all();

  /** Complete the current command and pop it. */
  void complete_current();

  /** Update active command (called per frame by instance). */
  void update();

  /** Draw UI for active command (called during ImGui phase). */
  void draw_ui();

  /** Draw 3D preview for active command (called during render phase). */
  void draw();

private:
  command_dispatcher() = default;
  ~command_dispatcher() = default;

  std::vector<std::unique_ptr<command>> m_command_stack;
};

} // namespace toy_cad::interaction
