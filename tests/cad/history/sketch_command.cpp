#include "sketch_command.h"

#include "sketch_feature.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/command_dispatcher.h"
#include "tests/cad/interaction/selection_manager.h"
#include "tests/cad/interaction/sketch_context.h"
#include "tests/cad/interaction/sketch_plane_input_handler.h"

#include "imgui.h"

namespace toy_cad {

sketch_command::sketch_command() : flow_command("sketch") {}

void sketch_command::cancel_nested_sketch_tools() {
  auto &dispatcher = interaction::command_dispatcher::instance();
  while (dispatcher.has_active_command()) {
    interaction::command *cmd = dispatcher.active_command();
    if (dynamic_cast<sketch_command *>(cmd) != nullptr) {
      break;
    }
    dispatcher.pop_command();
  }
}

void sketch_command::on_activate() {
  m_plane = gp_Ax2();
  m_sketch_handle = {};
  m_pending_sketch_exit = false;
  flow_command::on_activate();
}

void sketch_command::on_cancel() {
  cancel_nested_sketch_tools();

  if (sketch_context::instance().is_active()) {
    if (camera *cam = instance::get().scene_camera()) {
      sketch_context::instance().restore_camera(*cam);
    }
    sketch_context::instance().cancel();
  } else if (m_sketch_handle.valid()) {
    instance::get().db().destroy(m_sketch_handle);
    m_sketch_handle = {};
  }

  interaction::selection_manager::instance().clear_hover_target();
  sketch_context::instance().set_plane_hover(std::nullopt);
  flow_command::on_cancel();
}

void sketch_command::on_complete() {
  m_sketch_handle = {};
  interaction::selection_manager::instance().clear_hover_target();
  flow_command::on_complete();
}

bool sketch_command::on_update() {
  if (m_pending_sketch_exit) {
    m_pending_sketch_exit = false;
    cancel_nested_sketch_tools();
    if (camera *cam = instance::get().scene_camera()) {
      sketch_context::instance().restore_camera(*cam);
    }
    sketch_context::instance().finish();
    return false;
  }
  return flow_command::on_update();
}

void sketch_command::on_draw_ui() {
  flow_command::on_draw_ui();
  if (current_step_id() == "editing") {
    ImGui::Text("Click Exit Sketch when done");
  }
}

void sketch_command::on_step_entered(const interaction::step_def &step) {
  if (step.id == "editing") {
    if (camera *cam = instance::get().scene_camera()) {
      sketch_context::instance().begin(m_plane, m_sketch_handle, *cam);
    }
  }
}

void sketch_command::on_step_exited(const interaction::step_def &step) {
  if (step.id == "pick_plane") {
    interaction::selection_manager::instance().clear_hover_target();
    sketch_context::instance().set_plane_hover(std::nullopt);
  }
}

bool sketch_command::on_flow_action(const std::string &action,
                                    const interaction::flow_action_context &ctx) {
  if (action == "sketch.on_plane_confirmed") {
    auto *plane_handler =
        dynamic_cast<interaction::sketch_plane_input_handler *>(ctx.handler);
    if (!plane_handler || !plane_handler->has_valid_hover()) {
      return true;
    }

    const interaction::sketch_plane_pick pick = plane_handler->hover_pick();
    m_plane = pick.ax2;
    m_sketch_handle = instance::get().db().emplace<sketch_feature>();
    if (auto *sketch =
            instance::get().db().try_get_as<sketch_feature>(m_sketch_handle)) {
      sketch->set_plane(m_plane);
    }
    return false;
  }

  return false;
}

void sketch_command::request_exit() {
  if (current_step_id() != "editing") {
    return;
  }
  m_pending_sketch_exit = true;
}

} // namespace toy_cad
