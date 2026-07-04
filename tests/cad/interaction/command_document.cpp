#include "command_document.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace toy_cad::interaction {

namespace {

using json = nlohmann::json;

transition_mode parse_transition_mode(const json &node) {
  const std::string mode = node.value("mode", "deferred");
  return mode == "immediate" ? transition_mode::immediate
                             : transition_mode::deferred;
}

command_scope parse_scope(const json &node) {
  const std::string scope = node.value("scope", "global");
  return scope == "context" ? command_scope::context : command_scope::global;
}

completion_mode parse_completion(const json &node) {
  const std::string completion = node.value("completion", "explicit");
  return completion == "auto_update" ? completion_mode::auto_update
                                     : completion_mode::explicit_complete;
}

std::optional<transition_def> parse_transition(const json &node) {
  if (node.is_null() || !node.is_object()) {
    return std::nullopt;
  }

  transition_def out;
  if (node.contains("advance") && !node["advance"].is_null()) {
    out.advance = node["advance"].get<std::string>();
  }
  out.mode = parse_transition_mode(node);
  out.complete = node.value("complete", false);

  if (node.contains("actions") && node["actions"].is_array()) {
    for (const auto &action : node["actions"]) {
      out.actions.push_back(action.get<std::string>());
    }
  }

  if (node.contains("same_frame_fallback") &&
      node["same_frame_fallback"].is_object()) {
    same_frame_fallback_def fallback;
    fallback.when_step =
        node["same_frame_fallback"].value("when_step", std::string{});
    fallback.from_flag =
        node["same_frame_fallback"].value("from_flag", std::string{});
    if (!fallback.when_step.empty() && !fallback.from_flag.empty()) {
      out.same_frame_fallback = fallback;
    }
  }

  return out;
}

step_transitions_def parse_step_transitions(const json &node) {
  step_transitions_def out;
  if (!node.is_object()) {
    return out;
  }
  if (node.contains("confirm")) {
    out.confirm = parse_transition(node["confirm"]);
  }
  if (node.contains("preview")) {
    out.preview = parse_transition(node["preview"]);
  }
  if (node.contains("enter")) {
    out.enter = parse_transition(node["enter"]);
  }
  return out;
}

step_def parse_step(const json &node) {
  step_def out;
  out.id = node.at("id").get<std::string>();
  if (node.contains("handler") && !node["handler"].is_null()) {
    out.handler = node["handler"].get<std::string>();
  }
  out.flag = node.value("flag", std::string{});
  out.prompt = node.value("prompt", std::string{});
  out.sync_cursor = node.value("sync_cursor", false);
  if (node.contains("transitions")) {
    out.transitions = parse_step_transitions(node["transitions"]);
  }
  return out;
}

command_def parse_command(const json &node) {
  command_def out;
  out.id = node.at("id").get<std::string>();
  out.name = node.value("name", out.id);
  out.tooltip = node.value("tooltip", std::string{});
  out.icon = node.value("icon", std::string{"none"});
  out.scope = parse_scope(node);
  out.context_id = node.value("context", std::string{});
  out.impl = node.value("impl", out.id);
  out.completion = parse_completion(node);
  out.requires_key = node.value("requires", std::string{});

  if (node.contains("steps") && node["steps"].is_array()) {
    for (const auto &step_node : node["steps"]) {
      out.steps.push_back(parse_step(step_node));
    }
  }
  return out;
}

context_button_def parse_context_button(const json &node) {
  context_button_def out;
  out.label = node.at("label").get<std::string>();
  out.tooltip = node.value("tooltip", std::string{});
  out.icon = node.value("icon", std::string{"none"});
  if (node.contains("command") && !node["command"].is_null()) {
    out.command_id = node["command"].get<std::string>();
  }
  if (node.contains("action") && !node["action"].is_null()) {
    out.action = node["action"].get<std::string>();
  }
  return out;
}

context_def parse_context(const json &node) {
  context_def out;
  out.id = node.at("id").get<std::string>();
  out.name = node.value("name", out.id);
  out.enter_on_command = node.value("enter_on_command", std::string{});
  out.enter_on_step = node.value("enter_on_step", std::string{});
  if (node.contains("buttons") && node["buttons"].is_array()) {
    for (const auto &btn : node["buttons"]) {
      out.buttons.push_back(parse_context_button(btn));
    }
  }
  return out;
}

bool validate_document(const command_document &doc, std::string *error) {
  for (const command_def &cmd : doc.commands) {
    if (cmd.id.empty()) {
      if (error) {
        *error = "command entry missing id";
      }
      return false;
    }
    if (cmd.steps.empty()) {
      if (error) {
        *error = "command '" + cmd.id + "' has no steps";
      }
      return false;
    }
    if (cmd.scope == command_scope::context && cmd.context_id.empty()) {
      if (error) {
        *error = "context-scoped command '" + cmd.id + "' missing context";
      }
      return false;
    }
  }
  return true;
}

} // namespace

const command_def *command_document::find_command(const std::string &id) const {
  for (const command_def &cmd : commands) {
    if (cmd.id == id) {
      return &cmd;
    }
  }
  return nullptr;
}

const context_def *command_document::find_context(const std::string &id) const {
  for (const context_def &ctx : contexts) {
    if (ctx.id == id) {
      return &ctx;
    }
  }
  return nullptr;
}

const step_def *command_document::find_step(const command_def &cmd,
                                            const std::string &step_id) const {
  for (const step_def &step : cmd.steps) {
    if (step.id == step_id) {
      return &step;
    }
  }
  return nullptr;
}

bool load_command_document(command_document &out, std::string *error) {
  std::ifstream file("assets/cad/commands.json");
  if (!file.is_open()) {
    if (error) {
      *error = "failed to open assets/cad/commands.json";
    }
    return false;
  }

  json root;
  try {
    file >> root;
  } catch (const std::exception &e) {
    if (error) {
      *error = std::string("JSON parse error: ") + e.what();
    }
    return false;
  }

  out = command_document{};
  out.version = root.value("version", 0);

  if (root.contains("commands") && root["commands"].is_array()) {
    for (const auto &cmd_node : root["commands"]) {
      out.commands.push_back(parse_command(cmd_node));
    }
  }

  if (root.contains("contexts") && root["contexts"].is_array()) {
    for (const auto &ctx_node : root["contexts"]) {
      out.contexts.push_back(parse_context(ctx_node));
    }
  }

  if (!validate_document(out, error)) {
    return false;
  }

  std::cout << "[command_document] loaded " << out.commands.size()
            << " commands, " << out.contexts.size() << " contexts\n";
  return true;
}

} // namespace toy_cad::interaction
