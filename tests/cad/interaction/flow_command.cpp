#include "flow_command.h"

#include "command_actions.h"
#include "command_registry.h"
#include "extrude_depth_input_handler.h"
#include "face_input_handler.h"
#include "profile_input_handler.h"
#include "handler_registry.h"
#include "point_input_handler.h"
#include "sketch_plane_input_handler.h"
#include "sketch_point_input_handler.h"
#include "tests/cad/document/command_panel.h"
#include "tests/cad/instance.h"

#include "imgui.h"

#include <cstring>
#include <iostream>

namespace toy_cad::interaction {

namespace {

bool handler_has_valid_input(input_handler *handler, const step_def &step) {
  if (!handler || !step.handler.has_value()) {
    return true;
  }
  if (*step.handler == "point" || *step.handler == "sketch_point") {
    if (auto *point = dynamic_cast<point_input_handler *>(handler)) {
      return point->has_valid_preview();
    }
    if (auto *sketch_point =
            dynamic_cast<sketch_point_input_handler *>(handler)) {
      return sketch_point->has_valid_preview();
    }
    return false;
  }
  if (*step.handler == "face") {
    if (auto *face = dynamic_cast<face_input_handler *>(handler)) {
      return face->has_valid_hover();
    }
    return false;
  }
  if (*step.handler == "profile") {
    if (auto *profile = dynamic_cast<profile_input_handler *>(handler)) {
      return profile->has_valid_hover();
    }
    return false;
  }
  if (*step.handler == "extrude_depth") {
    if (auto *depth = dynamic_cast<extrude_depth_input_handler *>(handler)) {
      return depth->has_valid_preview();
    }
    return false;
  }
  if (*step.handler == "sketch_plane") {
    if (auto *plane = dynamic_cast<sketch_plane_input_handler *>(handler)) {
      return plane->has_valid_hover();
    }
    return false;
  }
  return true;
}

void sync_handler_cursor(input_handler *handler, const step_def &step) {
  if (!step.sync_cursor || !handler) {
    return;
  }
  if (auto *sketch_point = dynamic_cast<sketch_point_input_handler *>(handler)) {
    sketch_point->sync_from_viewport_cursor();
  } else if (auto *depth =
                 dynamic_cast<extrude_depth_input_handler *>(handler)) {
    depth->sync_from_viewport_cursor();
  }
}

} // namespace

flow_command::flow_command(std::string command_id)
    : m_command_id(std::move(command_id)) {
  m_def = command_registry::instance().find_command(m_command_id);
  if (!m_def) {
    std::cerr << "[flow_command] unknown command id: " << m_command_id << '\n';
  }
}

const char *flow_command::name() const {
  return m_def ? m_def->name.c_str() : m_command_id.c_str();
}

step_def *flow_command::current_step() {
  if (!m_def) {
    return nullptr;
  }
  for (step_def &step : const_cast<std::vector<step_def> &>(m_def->steps)) {
    if (step.id == m_current_step_id) {
      return &step;
    }
  }
  return nullptr;
}

input_handler *flow_command::active_handler() const {
  if (m_handler_step_id.empty()) {
    return nullptr;
  }
  const auto it = m_handlers.find(m_handler_step_id);
  return it != m_handlers.end() ? it->second.get() : nullptr;
}

void flow_command::pop_active_handler() {
  if (m_handler_step_id.empty()) {
    return;
  }
  const auto it = m_handlers.find(m_handler_step_id);
  if (it != m_handlers.end()) {
    instance::get().pop_input_handler();
    m_handlers.erase(it);
  }
  m_handler_step_id.clear();
}

void flow_command::on_activate() {
  m_current_step_id.clear();
  m_pending_handler_step.clear();
  m_pending_complete = false;
  m_pending_exit = false;
  m_context_pushed = false;
  m_active_context_id.clear();
  m_handlers.clear();
  m_handler_step_id.clear();

  if (!m_def || m_def->steps.empty()) {
    return;
  }

  m_current_step_id = m_def->steps.front().id;
  if (step_def *step = current_step()) {
    on_step_entered(*step);
    if (step->transitions.enter.has_value()) {
      execute_actions(step->transitions.enter->actions, nullptr, false, step);
    }
    if (step->handler.has_value()) {
      m_pending_handler_step = step->id;
    }
  }
}

void flow_command::on_cancel() {
  pop_active_handler();
  m_handlers.clear();
  if (m_context_pushed && !m_active_context_id.empty()) {
    pop_context_by_id(m_active_context_id);
    m_context_pushed = false;
    m_active_context_id.clear();
  }
}

void flow_command::on_complete() {
  pop_active_handler();
  m_handlers.clear();
  if (m_context_pushed && !m_active_context_id.empty()) {
    pop_context_by_id(m_active_context_id);
    m_context_pushed = false;
    m_active_context_id.clear();
  }
}

bool flow_command::execute_actions(const std::vector<std::string> &actions,
                                   input_handler *handler, bool confirmed,
                                   const step_def *step) {
  flow_action_context ctx{handler, confirmed, step};
  for (const std::string &action : actions) {
    if (command_actions::instance().invoke(action, *this, ctx)) {
      return true;
    }
    if (on_flow_action(action, ctx)) {
      return true;
    }
  }
  return false;
}

bool flow_command::on_update() {
  if (!m_pending_handler_step.empty()) {
    const std::string next_step_id = m_pending_handler_step;
    m_pending_handler_step.clear();

    pop_active_handler();

    if (step_def *prev = current_step()) {
      on_step_exited(*prev);
    }

    m_current_step_id = next_step_id;

    if (step_def *step = current_step()) {
      on_step_entered(*step);

      if (step->transitions.enter.has_value()) {
        execute_actions(step->transitions.enter->actions, nullptr, false,
                        step);
      }

      if (step->handler.has_value()) {
        auto handler = handler_registry::instance().create(
            *step->handler, this, step->flag);
        if (!handler) {
          std::cerr << "[flow_command] failed to create handler '"
                    << *step->handler << "' for step " << step->id << '\n';
        } else {
          m_handlers[step->id] = std::move(handler);
          m_handler_step_id = step->id;
          instance::get().push_input_handler(m_handlers[step->id].get());
          sync_handler_cursor(m_handlers[step->id].get(), *step);
          on_handler_attached(*step, m_handlers[step->id].get());
        }
      }
    }
  }

  if (m_pending_exit) {
    m_pending_exit = false;
    if (m_def && m_def->completion == completion_mode::auto_update) {
      return false;
    }
    command_dispatcher::instance().complete_current();
    return true;
  }

  if (m_pending_complete) {
    m_pending_complete = false;
    pop_active_handler();
    if (m_def && m_def->completion == completion_mode::explicit_complete) {
      command_dispatcher::instance().complete_current();
      return true;
    }
    return false;
  }

  return true;
}

void flow_command::on_draw_ui() {
  if (m_def) {
    ImGui::SeparatorText(m_def->name.c_str());
  }

  if (step_def *step = current_step()) {
    if (!step->prompt.empty()) {
      ImGui::TextUnformatted(step->prompt.c_str());
    }
  }

  if (ImGui::Button("Cancel")) {
    command_dispatcher::instance().pop_command();
  }
}

void flow_command::on_input_changed(const char *flag, input_handler *handler,
                                    bool confirmed) {
  if (!m_def || !handler || !flag) {
    return;
  }

  step_def *step =
      const_cast<step_def *>(
          command_registry::instance().document().find_step(*m_def,
                                                            m_current_step_id));
  if (!step) {
    return;
  }

  const auto flag_matches = [&](const step_def &s) { return s.flag == flag; };

  bool use_fallback = false;
  if (!flag_matches(*step)) {
    if (!confirmed) {
      return;
    }
    for (const step_def &candidate : m_def->steps) {
      if (!candidate.transitions.confirm.has_value() ||
          !candidate.transitions.confirm->same_frame_fallback.has_value()) {
        continue;
      }
      const auto &fb = *candidate.transitions.confirm->same_frame_fallback;
      if (fb.when_step == m_current_step_id && fb.from_flag == flag) {
        use_fallback = true;
        break;
      }
    }
    if (!use_fallback) {
      return;
    }
  }

  if (confirmed) {
    if (!step->transitions.confirm.has_value()) {
      return;
    }
    if (!handler_has_valid_input(handler, *step)) {
      return;
    }

    const transition_def &transition = *step->transitions.confirm;
    if (execute_actions(transition.actions, handler, true, step)) {
      return;
    }

    if (!transition.advance.empty() && transition.advance != "done") {
      m_current_step_id = transition.advance;
    }

    if (transition.complete || transition.advance == "done") {
      m_pending_complete = true;
      return;
    }

    if (!transition.advance.empty() && transition.advance != "done") {
      if (transition.mode == transition_mode::immediate) {
        m_pending_handler_step = transition.advance;
        on_update();
      } else {
        m_pending_handler_step = transition.advance;
      }
    }
    return;
  }

  if (step->transitions.preview.has_value()) {
    execute_actions(step->transitions.preview->actions, handler, false, step);
  }
}

void flow_command::request_exit() { m_pending_exit = true; }

void flow_command::push_context_by_id(const std::string &context_id) {
  command_registry::instance().push_context(context_id);
  m_context_pushed = true;
  m_active_context_id = context_id;
}

void flow_command::pop_context_by_id(const std::string &context_id) {
  command_registry::instance().pop_context(context_id);
}

} // namespace toy_cad::interaction
