#pragma once

#include "tests/cad/interaction/command_dispatcher.h"
#include "tests/cad/interaction/sketch_plane_input_handler.h"
#include "tests/cad/database/handle.h"

#include <gp_Ax2.hxx>
#include <memory>

namespace toy_cad {

/**
 * Enter sketch mode: pick support plane, draw curves, exit to keep profile.
 */
class sketch_command : public interaction::command {
public:
  sketch_command() = default;

  [[nodiscard]] const char *name() const override { return "Sketch"; }

  void on_activate() override;
  void on_cancel() override;
  void on_complete() override;

  bool on_update() override;

  void on_draw_ui() override;
  void on_draw() override;

  void on_input_changed(const char *flag, interaction::input_handler *handler,
                        bool confirmed) override;

  /** Finish sketch session and commit profile to history. */
  void exit_sketch();

  /** Cancel nested drawing commands before exit. */
  static void cancel_nested_sketch_tools();

private:
  enum class step { pick_plane, editing, done };

  step m_step = step::pick_plane;
  gp_Ax2 m_plane{};
  handle m_sketch_handle{};

  std::unique_ptr<interaction::sketch_plane_input_handler> m_plane_handler;

  bool m_pending_enter_editing = false;
  bool m_pending_exit = false;

  void setup_plane_handler();
  void on_plane_confirmed(const interaction::sketch_plane_pick &pick);
  void enter_editing_mode();
  void register_sketch_panel();
  void unregister_sketch_panel();
};

} // namespace toy_cad
