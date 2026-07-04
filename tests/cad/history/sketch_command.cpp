#include "sketch_command.h"

#include "sketch_circle_command.h"
#include "sketch_feature.h"
#include "sketch_line_command.h"
#include "tests/cad/document/command_panel.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/command_dispatcher.h"
#include "tests/cad/interaction/selection_manager.h"
#include "tests/cad/interaction/sketch_context.h"

#include "imgui.h"

#include <cstring>

namespace toy_cad {

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
  m_step = step::pick_plane;
  m_plane = gp_Ax2();
  m_sketch_handle = {};
  m_pending_enter_editing = false;
  m_pending_exit = false;
  setup_plane_handler();
}

void sketch_command::on_cancel() {
  cancel_nested_sketch_tools();
  unregister_sketch_panel();

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
}

void sketch_command::on_complete() {
  unregister_sketch_panel();
  m_sketch_handle = {};
  interaction::selection_manager::instance().clear_hover_target();
}

bool sketch_command::on_update() {
  if (m_pending_enter_editing) {
    m_pending_enter_editing = false;

    if (m_plane_handler) {
      instance::get().pop_input_handler();
      m_plane_handler.reset();
    }
    interaction::selection_manager::instance().clear_hover_target();
    sketch_context::instance().set_plane_hover(std::nullopt);
    enter_editing_mode();
  }

  if (m_pending_exit) {
    m_pending_exit = false;
    cancel_nested_sketch_tools();
    if (camera *cam = instance::get().scene_camera()) {
      sketch_context::instance().restore_camera(*cam);
    }
    sketch_context::instance().finish();
    m_step = step::done;
    return false;
  }

  return true;
}

void sketch_command::on_draw_ui() {
  ImGui::SeparatorText("Sketch");

  switch (m_step) {
  case step::pick_plane:
    ImGui::Text("Step 1: Click a datum plane or face");
    break;
  case step::editing:
    ImGui::Text("Sketch mode: use command panel to draw curves");
    ImGui::Text("Click Exit Sketch when done");
    break;
  case step::done:
    ImGui::Text("Sketch saved");
    break;
  }

  if (ImGui::Button("Cancel")) {
    interaction::command_dispatcher::instance().pop_command();
  }
}

void sketch_command::on_draw() {
  (void)m_step;
}

void sketch_command::on_input_changed(const char *flag,
                                      interaction::input_handler *handler,
                                      bool confirmed) {
  if (std::strcmp(flag, "plane") != 0 || !confirmed ||
      m_step != step::pick_plane) {
    return;
  }

  auto *plane_handler =
      dynamic_cast<interaction::sketch_plane_input_handler *>(handler);
  if (!plane_handler || !plane_handler->has_valid_hover()) {
    return;
  }

  on_plane_confirmed(plane_handler->hover_pick());
}

void sketch_command::setup_plane_handler() {
  m_plane_handler = std::make_unique<interaction::sketch_plane_input_handler>();
  m_plane_handler->set_owner(this, "plane");
  instance::get().push_input_handler(m_plane_handler.get());
}

void sketch_command::on_plane_confirmed(
    const interaction::sketch_plane_pick &pick) {
  m_plane = pick.ax2;
  m_sketch_handle = instance::get().db().emplace<sketch_feature>();
  if (auto *sketch =
          instance::get().db().try_get_as<sketch_feature>(m_sketch_handle)) {
    sketch->set_plane(m_plane);
  }

  m_pending_enter_editing = true;
}

void sketch_command::enter_editing_mode() {
  m_step = step::editing;

  if (camera *cam = instance::get().scene_camera()) {
    sketch_context::instance().begin(m_plane, m_sketch_handle, *cam);
  }

  register_sketch_panel();
}

void sketch_command::register_sketch_panel() {
  auto &panel = interaction::command_panel::instance();
  panel.push_context("Sketch");

  panel.register_context_button(
      "Exit Sketch", "Finish sketch and keep profile for extrusion",
      []() {
        auto &dispatcher = interaction::command_dispatcher::instance();
        if (auto *cmd = dynamic_cast<sketch_command *>(
                dispatcher.active_command())) {
          cmd->exit_sketch();
        }
      },
      interaction::ui_icon_id::none);

  panel.register_context_button(
      "Line", "Draw a line segment on the sketch plane",
      []() {
        if (!sketch_context::instance().is_active()) {
          return;
        }
        interaction::command_dispatcher::instance().push_command(
            std::make_unique<sketch_line_command>());
      },
      interaction::ui_icon_id::none);

  panel.register_context_button(
      "Circle", "Draw a circle on the sketch plane",
      []() {
        if (!sketch_context::instance().is_active()) {
          return;
        }
        interaction::command_dispatcher::instance().push_command(
            std::make_unique<sketch_circle_command>());
      },
      interaction::ui_icon_id::none);
}

void sketch_command::unregister_sketch_panel() {
  interaction::command_panel::instance().pop_context();
}

void sketch_command::exit_sketch() {
  if (m_step != step::editing) {
    return;
  }

  m_pending_exit = true;
}

} // namespace toy_cad
