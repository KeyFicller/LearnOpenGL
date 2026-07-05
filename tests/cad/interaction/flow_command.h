#pragma once

#include "command_document.h"
#include "command_dispatcher.h"
#include "input_handler.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace toy_cad::interaction {

struct flow_action_context {
  input_handler *handler = nullptr;
  bool confirmed = false;
  const step_def *step = nullptr;
};

/**
 * JSON-driven command step machine. Subclasses override on_flow_action for
 * domain logic; handler push/pop and deferred transitions are handled here.
 */
class flow_command : public command {
public:
  explicit flow_command(std::string command_id);

  flow_command(const flow_command &) = delete;
  flow_command &operator=(const flow_command &) = delete;

  const char *name() const override;
  const std::string &command_id() const { return m_command_id; }
  const command_def *def() const { return m_def; }
  const std::string &current_step_id() const {
    return m_current_step_id;
  }

  void on_activate() override;
  void on_cancel() override;
  void on_complete() override;
  bool on_update() override;
  void on_draw_ui() override;

  void on_input_changed(const char *flag, input_handler *handler,
                        bool confirmed) override;

  /** Return true to block transition after actions run. */
  virtual bool on_flow_action(const std::string &action,
                              const flow_action_context &ctx) {
    (void)action;
    (void)ctx;
    return false;
  }

  virtual void on_step_entered(const step_def &step) { (void)step; }
  virtual void on_step_exited(const step_def &step) { (void)step; }
  virtual void on_handler_attached(const step_def &step, input_handler *handler) {
    (void)step;
    (void)handler;
  }

  void request_exit();
  void push_context_by_id(const std::string &context_id);
  void pop_context_by_id(const std::string &context_id);

protected:
  bool execute_actions(const std::vector<std::string> &actions,
                       input_handler *handler, bool confirmed,
                       const step_def *step);
  void pop_active_handler();
  input_handler *active_handler() const;
  step_def *current_step();

  std::string m_command_id;
  const command_def *m_def = nullptr;

  std::string m_current_step_id;
  std::string m_pending_handler_step;
  bool m_pending_complete = false;
  bool m_pending_exit = false;
  bool m_context_pushed = false;
  std::string m_active_context_id;

  std::unordered_map<std::string, std::unique_ptr<input_handler>> m_handlers;
  std::string m_handler_step_id;
};

} // namespace toy_cad::interaction
