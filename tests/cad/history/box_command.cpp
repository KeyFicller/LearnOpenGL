#include "box_command.h"

#include "box_feature.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/ray_pick.h"
#include "tests/cad/interaction/command_dispatcher.h"
#include "tests/cad/database/database.h"
#include "imgui.h"

#include <GLFW/glfw3.h>

namespace toy_cad {

void box_command::on_activate() {
  m_step = step::pick_p1;
  m_p1 = glm::vec3(0.0f);
  m_p2 = glm::vec3(0.0f);
  m_preview_p2 = glm::vec3(0.0f);
  m_box_handle = {};
}

void box_command::on_cancel() {
  // Clean up preview box if cancelled (not yet in history)
  if (m_box_handle.valid()) {
    instance::get().db().destroy(m_box_handle);
    m_box_handle = {};
  }
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
      // Call regen() to ensure topology is up to date, then draw
      box->regen();
      box->draw_global();
    }
  }
}

bool box_command::on_mouse_moved(double xpos, double ypos) {
  if (m_step == step::pick_p2) {
    update_preview(xpos, ypos);
    return true;
  }
  return false;
}

bool box_command::on_mouse_button(int button, int action, int mods) {
  (void)mods;

  if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) {
    return false;
  }

  switch (m_step) {
  case step::pick_p1: {
    // Get world point at mouse position
    auto &picker = interaction::ray_pick::instance();
    glm::vec3 world_point;
    if (!picker.pick_at(m_mouse_x, m_mouse_y, instance::get().disp(), world_point)) {
      return false; // No valid intersection
    }

    m_p1 = world_point;
    m_preview_p2 = world_point;

    // Create preview box feature (NOT added to history yet)
    m_box_handle = instance::get().db().emplace<box_feature>();
    if (auto *box = instance::get().db().try_get_as<box_feature>(m_box_handle)) {
      box->set_corners(m_p1, m_preview_p2);
      box->regen(); // Generate initial topology
    }

    m_step = step::pick_p2;
    return true;
  }

  case step::pick_p2: {
    m_p2 = m_preview_p2;

    // Finalize box geometry
    if (m_box_handle.valid()) {
      if (auto *box = instance::get().db().try_get_as<box_feature>(m_box_handle)) {
        box->set_corners(m_p1, m_p2);
        box->regen(); // Final topology generation
      }

      // NOW add to history (confirmed)
      instance::get().history().push_back(m_box_handle);
    }

    m_step = step::done;

    // Complete command
    interaction::command_dispatcher::instance().complete_current();
    return true;
  }

  case step::done:
    return false;
  }

  return false;
}

void box_command::update_preview(double screen_x, double screen_y) {
  m_mouse_x = screen_x;
  m_mouse_y = screen_y;

  auto &picker = interaction::ray_pick::instance();
  glm::vec3 world_point;
  if (picker.pick_at(screen_x, screen_y, instance::get().disp(), world_point)) {
    m_preview_p2 = world_point;

    // Update preview box corners (topology will be regenerated in on_draw)
    if (m_box_handle.valid()) {
      if (auto *box = instance::get().db().try_get_as<box_feature>(m_box_handle)) {
        box->set_corners(m_p1, m_preview_p2);
      }
    }
  }
}

void box_command::commit_box() {
  // Box is already created and updated during interaction
  // Final confirmation happens in pick_p2 when we push to history
}

} // namespace toy_cad
