#pragma once

#include "flow_command.h"

#include <functional>
#include <string>
#include <unordered_map>

namespace toy_cad::interaction {

/** Global action hooks referenced from commands.json. */
class command_actions {
public:
  using action_fn = std::function<bool(flow_command &, const flow_action_context &)>;

  static command_actions &instance();

  command_actions(const command_actions &) = delete;
  command_actions &operator=(const command_actions &) = delete;

  void register_action(const std::string &id, action_fn fn);
  [[nodiscard]] bool invoke(const std::string &id, flow_command &cmd,
                            const flow_action_context &ctx);

  void register_builtin_actions();

private:
  command_actions() = default;

  std::unordered_map<std::string, action_fn> m_actions;
};

} // namespace toy_cad::interaction
