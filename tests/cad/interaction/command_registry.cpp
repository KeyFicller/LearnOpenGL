#include "command_registry.h"

#include "command_actions.h"
#include "command_dispatcher.h"
#include "flow_command.h"
#include "sketch_context.h"
#include "tests/cad/document/command_panel.h"
#include "tests/cad/document/ui_icons.h"

#include <iostream>

namespace toy_cad::interaction {

command_registry &command_registry::instance() {
  static command_registry s;
  return s;
}

ui_icon_id command_registry::parse_icon(const std::string &icon) const {
  if (icon == "box") {
    return ui_icon_id::box;
  }
  if (icon == "sketch") {
    return ui_icon_id::sketch;
  }
  if (icon == "extrude") {
    return ui_icon_id::extrude;
  }
  return ui_icon_id::none;
}

bool command_registry::load_and_register(std::string *error) {
  if (!load_command_document(m_document, error)) {
    return false;
  }

  command_actions::instance().register_builtin_actions();

  auto &panel = command_panel::instance();
  panel.clear_buttons();

  for (const command_def &cmd : m_document.commands) {
    if (cmd.scope != command_scope::global) {
      continue;
    }

    const std::string cmd_id = cmd.id;
    panel.register_button(
        cmd.name.c_str(), cmd.tooltip.c_str(),
        [cmd_id]() { command_registry::instance().push_command(cmd_id); },
        parse_icon(cmd.icon));
  }

  m_loaded = true;
  return true;
}

void command_registry::register_impl_factory(const std::string &impl,
                                             command_factory_fn fn) {
  m_impl_factories[impl] = std::move(fn);
}

std::unique_ptr<command> command_registry::create_command(const std::string &id) {
  const command_def *def = find_command(id);
  if (!def) {
    std::cerr << "[command_registry] unknown command id: " << id << '\n';
    return nullptr;
  }

  const auto it = m_impl_factories.find(def->impl);
  if (it == m_impl_factories.end()) {
    std::cerr << "[command_registry] no factory for impl: " << def->impl << '\n';
    return nullptr;
  }

  return it->second();
}

void command_registry::push_command(const std::string &id) {
  const command_def *def = find_command(id);
  if (!def) {
    return;
  }
  if (!def->requires_key.empty() && !check_requires(def->requires_key)) {
    return;
  }

  auto cmd = create_command(id);
  if (cmd) {
    command_dispatcher::instance().push_command(std::move(cmd));
  }
}

const command_def *command_registry::find_command(const std::string &id) const {
  return m_document.find_command(id);
}

bool command_registry::check_requires(const std::string &requires_key) const {
  if (requires_key == "sketch_context.active") {
    return sketch_context::instance().is_active();
  }
  return true;
}

void command_registry::push_context(const std::string &context_id) {
  const context_def *ctx = m_document.find_context(context_id);
  if (!ctx) {
    std::cerr << "[command_registry] unknown context: " << context_id << '\n';
    return;
  }

  auto &panel = command_panel::instance();
  panel.push_context(ctx->name.c_str());

  for (const context_button_def &btn : ctx->buttons) {
    if (btn.command_id.has_value()) {
      const std::string cmd_id = *btn.command_id;
      panel.register_context_button(
          btn.label.c_str(), btn.tooltip.c_str(),
          [cmd_id]() { command_registry::instance().push_command(cmd_id); },
          parse_icon(btn.icon));
    } else if (btn.action.has_value()) {
      const std::string action = *btn.action;
      panel.register_context_button(
          btn.label.c_str(), btn.tooltip.c_str(),
          [action]() {
            flow_command *flow = dynamic_cast<flow_command *>(
                command_dispatcher::instance().active_command());
            if (!flow) {
              return;
            }
            flow_action_context ctx;
            (void)command_actions::instance().invoke(action, *flow, ctx);
          },
          parse_icon(btn.icon));
    }
  }
}

void command_registry::pop_context(const std::string &context_id) {
  (void)context_id;
  command_panel::instance().pop_context();
}

} // namespace toy_cad::interaction
