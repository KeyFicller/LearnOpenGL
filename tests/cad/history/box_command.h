#pragma once

#include "tests/cad/interaction/flow_command.h"
#include "tests/cad/database/handle.h"

#include <glm/glm.hpp>

namespace toy_cad {

class box_command : public interaction::flow_command {
public:
  box_command();

  void on_cancel() override;
  void on_complete() override;
  void on_draw_ui() override;
  void on_draw() override;

  bool on_flow_action(const std::string &action,
                      const interaction::flow_action_context &ctx) override;

private:
  glm::vec3 m_p1{0.0f};
  glm::vec3 m_p2{0.0f};
  glm::vec3 m_preview_p2{0.0f};
  handle m_box_handle{};
};

} // namespace toy_cad
