#pragma once

#include "input_handler.h"

#include <memory>
#include <string>

namespace toy_cad::interaction {

class flow_command;

/** Creates input handlers by JSON type string. */
class handler_registry {
public:
  static handler_registry &instance();

  handler_registry(const handler_registry &) = delete;
  handler_registry &operator=(const handler_registry &) = delete;

  [[nodiscard]] bool is_known_type(const std::string &type) const;

  [[nodiscard]] std::unique_ptr<input_handler>
  create(const std::string &type, flow_command *owner, const std::string &flag);

private:
  handler_registry() = default;
};

} // namespace toy_cad::interaction
