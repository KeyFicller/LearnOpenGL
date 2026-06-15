#include "box_command.h"

#include "box_feature.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/command_dispatcher.h"
#include "tests/cad/database/database.h"
#include "imgui.h"

#include <cstring>

namespace toy_cad {

void box_command::on_activate() {
  m_step = step::pick_p1;
  m_p1 = glm::vec3(0.0f);
  m_p2 = glm::vec3(0.0f);
  m_preview_p2 = glm::vec3(0.0f);
  m_box_handle = {};

  // Push first point handler
  setup_p1_handler();
}

void box_command::on_cancel() {
  // Clean up preview box if cancelled
  if (m_box_handle.valid()) {
    instance::get().db().destroy(m_box_handle);
    m_box_handle = {};
  }
  // Note: handlers are popped by instance's input stack management
}

void box_command::on_complete() {
  // Box is now owned by history, clear our handle
  m_box_handle = {};
}

void box_command::on_draw_ui() {
  ImGui::SeparatorText("Create Box");

  switch (m_step) {
  case step::pick_p1:
    ImGui::Text("Step 1: Click to set first corner point");
    break;
  case step::pick_p2:
    ImGui::Text("Step 2: Click to set opposite corner");
    ImGui::Text("P1: (%.3f, %.3f, %.3f)", m_p1.x, m_p1.y, m_p1.z);
    ImGui::Text("Preview: (%.3f, %.3f, %.3f)", m_preview_p2.x, m_preview_p2.y, m_preview_p2.z);
    break;
  case step::done:
    ImGui::Text("Box created!");
    break;
  }

  if (ImGui::Button("Cancel")) {
    interaction::command_dispatcher::instance().pop_command();
  }
}

void box_command::on_draw() {
  // Draw preview box during step 2
  if (m_step == step::pick_p2 && m_box_handle.valid()) {
    if (auto *box = instance::get().db().try_get_as<box_feature>(m_box_handle)) {
      box->regen();
      box->draw_global();
    }
  }
}

void box_command::on_input_changed(const char *flag, interaction::input_handler *handler, bool confirmed) {
  auto *point_handler = dynamic_cast<interaction::point_input_handler *>(handler);
  if (!point_handler) {
    return;
  }

  if (!point_handler->has_valid_preview()) {
    return;
  }

  glm::vec3 point = point_handler->preview_point();

  if (std::strcmp(flag, "p1") == 0) {
    if (confirmed && m_step == step::pick_p1) {
      // Click confirmed p1
      on_p1_confirmed(point);
    }
  } else if (std::strcmp(flag, "p2") == 0) {
    if (m_step == step::pick_p2) {
      if (confirmed) {
        // Click confirmed p2
        on_p2_confirmed(point);
      } else {
        // Preview update
        m_preview_p2 = point;
        if (m_box_handle.valid()) {
          if (auto *box = instance::get().db().try_get_as<box_feature>(m_box_handle)) {
            box->set_corners(m_p1, m_preview_p2);
          }
        }
      }
    }
  }
}

void box_command::setup_p1_handler() {
  m_p1_handler = std::make_unique<interaction::point_input_handler>();
  m_p1_handler->set_owner(this, "p1");
  instance::get().push_input_handler(m_p1_handler.get());
}

void box_command::setup_p2_handler() {
  m_p2_handler = std::make_unique<interaction::point_input_handler>();
  m_p2_handler->set_owner(this, "p2");
  instance::get().push_input_handler(m_p2_handler.get());
}

void box_command::on_p1_confirmed(const glm::vec3 &point) {
  m_p1 = point;
  m_preview_p2 = point;

  // Create preview box feature
  m_box_handle = instance::get().db().emplace<box_feature>();
  if (auto *box = instance::get().db().try_get_as<box_feature>(m_box_handle)) {
    box->set_corners(m_p1, m_preview_p2);
    box->regen();
  }

  // Pop p1 handler and push p2 handler
  instance::get().pop_input_handler();
  m_p1_handler.reset();

  m_step = step::pick_p2;
  setup_p2_handler();
}

void box_command::on_p2_confirmed(const glm::vec3 &point) {
  m_p2 = point;

  // Finalize box
  if (m_box_handle.valid()) {
    if (auto *box = instance::get().db().try_get_as<box_feature>(m_box_handle)) {
      box->set_corners(m_p1, m_p2);
      box->regen();
    }

    // Add to history
    instance::get().history().push_back(m_box_handle);
  }

  // Pop p2 handler
  instance::get().pop_input_handler();
  m_p2_handler.reset();

  m_step = step::done;

  // Complete command
  interaction::command_dispatcher::instance().complete_current();
}

} // namespace toy_cad
