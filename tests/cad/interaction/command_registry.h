#pragma once

#include "command_document.h"
#include "tests/cad/document/ui_icons.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace toy_cad::interaction {

class command;

using command_factory_fn = std::function<std::unique_ptr<command>()>;

/** Loads commands.json, registers panel buttons, creates commands by id. */
class command_registry {
public:
  static command_registry &instance();

  command_registry(const command_registry &) = delete;
  command_registry &operator=(const command_registry &) = delete;

  [[nodiscard]] bool load_and_register(std::string *error = nullptr);

  void register_impl_factory(const std::string &impl, command_factory_fn fn);

  [[nodiscard]] std::unique_ptr<command> create_command(const std::string &id);
  void push_command(const std::string &id);

  [[nodiscard]] const command_def *find_command(const std::string &id) const;
  [[nodiscard]] const command_document &document() const { return m_document; }

  void push_context(const std::string &context_id);
  void pop_context(const std::string &context_id);

  [[nodiscard]] bool check_requires(const std::string &requires_key) const;

private:
  command_registry() = default;

  ui_icon_id parse_icon(const std::string &icon) const;

  command_document m_document;
  std::unordered_map<std::string, command_factory_fn> m_impl_factories;
  bool m_loaded = false;
};

} // namespace toy_cad::interaction
