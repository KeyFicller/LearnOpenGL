#pragma once

#include "tests/cad/interaction/flow_command.h"
#include "tests/cad/interaction/sketch_plane_input_handler.h"
#include "tests/cad/database/handle.h"

#include <gp_Ax2.hxx>

namespace toy_cad {

class sketch_command : public interaction::flow_command {
public:
  sketch_command();

  void on_activate() override;
  void on_cancel() override;
  void on_complete() override;
  bool on_update() override;
  void on_draw_ui() override;

  bool on_flow_action(const std::string &action,
                      const interaction::flow_action_context &ctx) override;
  void on_step_entered(const interaction::step_def &step) override;
  void on_step_exited(const interaction::step_def &step) override;

  void request_exit();

  static void cancel_nested_sketch_tools();

private:
  gp_Ax2 m_plane{};
  handle m_sketch_handle{};
  handle m_plane_source_handle{};
};

} // namespace toy_cad
