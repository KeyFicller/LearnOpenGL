#include "command_dispatcher.h"

namespace toy_cad::interaction {

command_dispatcher &command_dispatcher::instance() {
  static command_dispatcher s;
  return s;
}

bool command_dispatcher::has_active_command() const {
  return !m_command_stack.empty();
}

command *command_dispatcher::active_command() const {
  return m_command_stack.empty() ? nullptr : m_command_stack.back().get();
}

void command_dispatcher::push_command(std::unique_ptr<command> cmd) {
  if (cmd) {
    m_command_stack.push_back(std::move(cmd));
    m_command_stack.back()->on_activate();
  }
}

void command_dispatcher::pop_command() {
  if (!m_command_stack.empty()) {
    m_command_stack.back()->on_cancel();
    m_command_stack.pop_back();
  }
}

void command_dispatcher::cancel_all() {
  while (!m_command_stack.empty()) {
    m_command_stack.back()->on_cancel();
    m_command_stack.pop_back();
  }
}

void command_dispatcher::complete_current() {
  if (!m_command_stack.empty()) {
    m_command_stack.back()->on_complete();
    m_command_stack.pop_back();
  }
}

void command_dispatcher::update() {
  if (!m_command_stack.empty()) {
    bool keep_alive = m_command_stack.back()->on_update();
    if (!keep_alive) {
      complete_current();
    }
  }
}

void command_dispatcher::draw_ui() {
  if (!m_command_stack.empty()) {
    m_command_stack.back()->on_draw_ui();
  }
}

void command_dispatcher::draw() {
  if (!m_command_stack.empty()) {
    m_command_stack.back()->on_draw();
  }
}

} // namespace toy_cad::interaction
