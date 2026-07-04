#include "command_actions.h"

#include "command_dispatcher.h"
#include "command_registry.h"
#include "sketch_context.h"
#include "tests/cad/history/sketch_command.h"

namespace toy_cad::interaction {

command_actions &command_actions::instance() {
  static command_actions s;
  return s;
}

void command_actions::register_action(const std::string &id, action_fn fn) {
  m_actions[id] = std::move(fn);
}

bool command_actions::invoke(const std::string &id, flow_command &cmd,
                             const flow_action_context &ctx) {
  const auto it = m_actions.find(id);
  if (it == m_actions.end()) {
    return false;
  }
  return it->second(cmd, ctx);
}

void command_actions::register_builtin_actions() {
  register_action("sketch.request_exit", [](flow_command &,
                                            const flow_action_context &) {
    auto *cmd = command_dispatcher::instance().find_command(
        [](const command *c) { return dynamic_cast<const sketch_command *>(c) != nullptr; });
    if (auto *sketch = dynamic_cast<sketch_command *>(cmd)) {
      sketch->request_exit();
      return true;
    }
    return false;
  });

  register_action("sketch.enter_editing", [](flow_command &cmd,
                                             const flow_action_context &) {
    if (cmd.command_id() != "sketch") {
      return false;
    }
    cmd.push_context_by_id("sketch");
    return true;
  });
}

} // namespace toy_cad::interaction
