#include "doc_input_handler.h"

#include "tests/cad/instance.h"
#include "tests/cad/interaction/ray_pick.h"
#include "tests/cad/interaction/selection_manager.h"
#include "tests/cad/renderer/disp_var.h"
#include "tests/framework/test_suit.h"

#include "imgui.h"
#include <GLFW/glfw3.h>

#include <cstdio>

namespace toy_cad::interaction {

namespace {

glm::vec2 viewport_relative_cursor(GLFWwindow *window) {
  if (!window) {
    return glm::vec2(0.0f);
  }
  double x = 0.0;
  double y = 0.0;
  glfwGetCursorPos(window, &x, &y);
  auto *test_suit_ptr =
      static_cast<test_suit *>(glfwGetWindowUserPointer(window));
  if (test_suit_ptr) {
    x -= test_suit_ptr->m_viewport_x;
    y -= test_suit_ptr->m_viewport_y;
  }
  return glm::vec2(static_cast<float>(x), static_cast<float>(y));
}

} // namespace

doc_input_handler &doc_input_handler::instance() {
  static doc_input_handler doc;
  return doc;
}

void doc_input_handler::clear_selection() {
  m_selection.clear();
  // Also clear subshape selection
  selection_manager::instance().clear_selection();
}

bool doc_input_handler::is_selected(handle h) const {
  return m_selection.find(h) != m_selection.end();
}

void doc_input_handler::on_explorer_item_left_click(handle item) {
  if (!item.valid()) {
    return;
  }
  ImGuiIO &io = ImGui::GetIO();
  if (io.KeyCtrl) {
    if (is_selected(item)) {
      m_selection.erase(item);
    } else {
      m_selection.insert(item);
    }
  } else {
    m_selection.clear();
    m_selection.insert(item);
  }
}

bool doc_input_handler::on_mouse_moved(double xpos, double ypos) {
  m_last_mouse_pos = glm::vec2(static_cast<float>(xpos),
                               static_cast<float>(ypos));

  if (m_is_dragging) {
    update_drag_selection(m_last_mouse_pos);
  } else if (m_left_button_down) {
    maybe_begin_drag_from_press(m_last_mouse_pos);
    if (m_is_dragging) {
      update_drag_selection(m_last_mouse_pos);
    }
  } else {
    update_hover(xpos, ypos);
  }

  return false;
}

bool doc_input_handler::on_mouse_button(int button, int action, int mods) {
  (void)mods;
  ImGuiIO &io = ImGui::GetIO();

  if (button != GLFW_MOUSE_BUTTON_LEFT) {
    return false;
  }

  if (action == GLFW_PRESS) {
    m_last_mouse_pos = viewport_relative_cursor(instance::get().window());
    m_left_button_down = true;
    m_press_pos = m_last_mouse_pos;
    return true;
  }

  if (action == GLFW_RELEASE) {
    if (!m_left_button_down && !m_is_dragging) {
      return false;
    }

    m_last_mouse_pos = viewport_relative_cursor(instance::get().window());

    const bool multi = io.KeyCtrl;
    if (m_is_dragging) {
      end_drag_selection(multi);
    } else {
      handle_click_selection(m_last_mouse_pos.x, m_last_mouse_pos.y, multi);
    }
    m_left_button_down = false;
    return true;
  }

  return false;
}

std::optional<std::pair<glm::vec2, glm::vec2>>
doc_input_handler::drag_rect() const {
  if (!m_is_dragging) {
    return std::nullopt;
  }
  return std::make_pair(
      glm::min(m_drag_start, m_drag_current),
      glm::max(m_drag_start, m_drag_current));
}

void doc_input_handler::update_hover(double xpos, double ypos) {
  auto &disp = instance::get().disp();
  auto &picker = ray_pick::instance();
  auto &sel_mgr = selection_manager::instance();

  // Generate ray from mouse position
  const auto ray = picker.screen_to_world_ray(xpos, ypos, disp);

  // Update hover state
  sel_mgr.update_hover(ray.origin, ray.direction);
}

void doc_input_handler::handle_click_selection(double xpos, double ypos,
                                               bool multi) {
  auto &disp = instance::get().disp();
  auto &picker = ray_pick::instance();
  auto &sel_mgr = selection_manager::instance();

  // Generate ray
  const auto ray = picker.screen_to_world_ray(xpos, ypos, disp);

  // Perform ray cast
  const auto hit = sel_mgr.hover_target();

  if (hit.has_value()) {
    // Toggle selection on the hit subshape
    sel_mgr.toggle_select(hit.value(), multi);

    // Debug output
    std::printf("[doc_input_handler] Selected subshape (type=%s, idx=%u) from feature %u\n",
                hit->subshape.is_face() ? "face" : "edge",
                hit->subshape.id().index,
                hit->feature_handle.index);
  } else {
    // Clicked on empty space - clear selection if not multi
    if (!multi) {
      sel_mgr.clear_selection();
      std::printf("[doc_input_handler] Cleared selection\n");
    }
  }
}

void doc_input_handler::maybe_begin_drag_from_press(
    const glm::vec2 &current_pos) {
  if (!m_left_button_down || m_is_dragging) {
    return;
  }
  const glm::vec2 delta = current_pos - m_press_pos;
  if (glm::length(delta) < k_box_drag_threshold_px) {
    return;
  }
  begin_drag_selection(m_press_pos);
}

void doc_input_handler::begin_drag_selection(const glm::vec2 &start_pos) {
  m_is_dragging = true;
  m_drag_start = start_pos;
  m_drag_current = start_pos;

  selection_manager::instance().begin_drag_select(start_pos);
}

void doc_input_handler::update_drag_selection(const glm::vec2 &current_pos) {
  if (!m_is_dragging) {
    return;
  }

  m_drag_current = current_pos;

  auto &disp = instance::get().disp();
  auto &sel_mgr = selection_manager::instance();

  // Update preview selection
  const bool changed = sel_mgr.update_drag_select(
      current_pos,
      glm::vec2(static_cast<float>(disp.render_width),
                static_cast<float>(disp.render_height)),
      disp.clip_from_world());

  (void)changed;
}

void doc_input_handler::end_drag_selection(bool multi) {
  if (!m_is_dragging) {
    return;
  }

  auto &sel_mgr = selection_manager::instance();
  sel_mgr.end_drag_select(multi);
  m_is_dragging = false;
}

void doc_input_handler::cancel_drag_selection() {
  if (!m_is_dragging) {
    return;
  }

  auto &sel_mgr = selection_manager::instance();
  sel_mgr.cancel_drag_select();

  m_is_dragging = false;
}

} // namespace toy_cad::interaction
