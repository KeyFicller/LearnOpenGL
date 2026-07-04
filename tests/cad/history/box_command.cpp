#include "box_command.h"

#include "box_feature.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/point_input_handler.h"

#include "imgui.h"

namespace toy_cad {

box_command::box_command() : flow_command("box") {}

void box_command::on_cancel() {
  if (m_box_handle.valid()) {
    instance::get().db().destroy(m_box_handle);
    m_box_handle = {};
  }
  flow_command::on_cancel();
}

void box_command::on_complete() {
  m_box_handle = {};
  flow_command::on_complete();
}

void box_command::on_draw_ui() {
  flow_command::on_draw_ui();

  if (current_step_id() == "pick_p2") {
    ImGui::Text("P1: (%.3f, %.3f, %.3f)", m_p1.x, m_p1.y, m_p1.z);
    ImGui::Text("Preview: (%.3f, %.3f, %.3f)", m_preview_p2.x, m_preview_p2.y,
                m_preview_p2.z);
  }
}

void box_command::on_draw() {
  if (current_step_id() == "pick_p2" && m_box_handle.valid()) {
    if (auto *box = instance::get().db().try_get_as<box_feature>(m_box_handle)) {
      box->regen();
      box->draw_global();
    }
  }
}

bool box_command::on_flow_action(const std::string &action,
                                 const interaction::flow_action_context &ctx) {
  auto *point_handler =
      dynamic_cast<interaction::point_input_handler *>(ctx.handler);
  if (!point_handler) {
    return false;
  }

  if (action == "box.on_p1_confirmed") {
    if (!ctx.confirmed || !point_handler->has_valid_preview()) {
      return true;
    }
    m_p1 = point_handler->preview_point();
    m_preview_p2 = m_p1;
    m_box_handle = instance::get().db().emplace<box_feature>();
    if (auto *box = instance::get().db().try_get_as<box_feature>(m_box_handle)) {
      box->set_corners(m_p1, m_preview_p2);
      box->regen();
    }
    return false;
  }

  if (action == "box.on_p2_preview") {
    if (ctx.confirmed || !point_handler->has_valid_preview()) {
      return false;
    }
    m_preview_p2 = point_handler->preview_point();
    if (m_box_handle.valid()) {
      if (auto *box =
              instance::get().db().try_get_as<box_feature>(m_box_handle)) {
        box->set_corners(m_p1, m_preview_p2);
      }
    }
    return false;
  }

  if (action == "box.on_p2_confirmed") {
    if (!ctx.confirmed || !point_handler->has_valid_preview()) {
      return true;
    }
    m_p2 = point_handler->preview_point();
    if (m_box_handle.valid()) {
      if (auto *box =
              instance::get().db().try_get_as<box_feature>(m_box_handle)) {
        box->set_corners(m_p1, m_p2);
        box->regen();
      }
      instance::get().history().push_back(m_box_handle);
    }
    return false;
  }

  return false;
}

} // namespace toy_cad
