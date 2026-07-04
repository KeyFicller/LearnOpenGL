#pragma once

#include <optional>
#include <string>
#include <vector>

namespace toy_cad::interaction {

enum class transition_mode { immediate, deferred };

enum class command_scope { global, context };

enum class completion_mode { explicit_complete, auto_update };

struct same_frame_fallback_def {
  std::string when_step;
  std::string from_flag;
};

struct transition_def {
  std::string advance;
  transition_mode mode = transition_mode::deferred;
  std::vector<std::string> actions;
  bool complete = false;
  std::optional<same_frame_fallback_def> same_frame_fallback;
};

struct step_transitions_def {
  std::optional<transition_def> confirm;
  std::optional<transition_def> preview;
  std::optional<transition_def> enter;
};

struct step_def {
  std::string id;
  std::optional<std::string> handler;
  std::string flag;
  std::string prompt;
  bool sync_cursor = false;
  step_transitions_def transitions;
};

struct command_def {
  std::string id;
  std::string name;
  std::string tooltip;
  std::string icon;
  command_scope scope = command_scope::global;
  std::string context_id;
  std::string impl;
  completion_mode completion = completion_mode::explicit_complete;
  std::string requires_key;
  std::vector<step_def> steps;
};

struct context_button_def {
  std::string label;
  std::string tooltip;
  std::string icon;
  std::optional<std::string> command_id;
  std::optional<std::string> action;
};

struct context_def {
  std::string id;
  std::string name;
  std::string enter_on_command;
  std::string enter_on_step;
  std::vector<context_button_def> buttons;
};

struct command_document {
  int version = 0;
  std::vector<command_def> commands;
  std::vector<context_def> contexts;

  [[nodiscard]] const command_def *find_command(const std::string &id) const;
  [[nodiscard]] const context_def *find_context(const std::string &id) const;
  [[nodiscard]] const step_def *find_step(const command_def &cmd,
                                          const std::string &step_id) const;
};

/** Load and validate assets/cad/commands.json (relative to resource root). */
[[nodiscard]] bool load_command_document(command_document &out,
                                         std::string *error = nullptr);

} // namespace toy_cad::interaction
