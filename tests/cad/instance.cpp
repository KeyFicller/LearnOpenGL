#include "instance.h"

#include "tests/cad/database/axis.h"
#include "tests/cad/database/datum.h"
#include "tests/cad/history/coordinate.h"
#include "tests/cad/interaction/doc_input_handler.h"
#include "tests/cad/interaction/inspector.h"

#include <array>

namespace toy_cad {

void instance::push_input_handler(interaction::input_handler *handler) {
  m_input_stack.push_back(handler);
}

void instance::pop_input_handler() {
  if (!m_input_stack.empty()) {
    m_input_stack.pop_back();
  }
}

interaction::input_handler *instance::top_input_handler() {
  return m_input_stack.empty() ? nullptr : m_input_stack.back();
}

void instance::init(GLFWwindow *window) {
  m_window = window;
  m_database.clear();
  m_history.clear();
  m_input_stack.clear();

  interaction::doc_input_handler::instance().clear_selection();
  interaction::inspector::instance().clear_target();
  push_input_handler(&interaction::doc_input_handler::instance());

  std::array<handle, 3> dh{};
  std::array<handle, 3> ah{};
  for (int i = 0; i < 3; ++i) {
    dh[i] = m_database.emplace<datum>();
    ah[i] = m_database.emplace<axis>();
  }
  const handle coord = m_database.emplace<coordinate>(dh, ah);
  m_history.push_back(coord);
}

void instance::update(float delta_time) { (void)delta_time; }

void instance::render() {
  m_history.draw_global();
  m_history.draw_local();
}

void instance::render_ui() {
  m_history.draw_ui();
  interaction::inspector::instance().draw_ui();
}

bool instance::on_mouse_moved(double xpos, double ypos) {
  if (auto *top = top_input_handler()) {
    return top->on_mouse_moved(xpos, ypos);
  }
  return false;
}

bool instance::on_mouse_scroll(double xoffset, double yoffset) {
  if (auto *top = top_input_handler()) {
    return top->on_mouse_scroll(xoffset, yoffset);
  }
  return false;
}

bool instance::on_mouse_button(int button, int action, int mods) {
  if (auto *top = top_input_handler()) {
    return top->on_mouse_button(button, action, mods);
  }
  return false;
}

void instance::on_framebuffer_resized(int width, int height) {
  if (auto *top = top_input_handler()) {
    top->on_framebuffer_resized(width, height);
  }
}

void instance::on_object_hovered(int object_id) { (void)object_id; }

} // namespace toy_cad
