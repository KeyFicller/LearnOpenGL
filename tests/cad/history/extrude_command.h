#pragma once

#include "tests/cad/interaction/flow_command.h"
#include "tests/cad/database/handle.h"

#include <glm/glm.hpp>
#include <TopoDS_Face.hxx>

namespace toy_cad {

class extrude_command : public interaction::flow_command {
public:
  extrude_command();

  void on_activate() override;
  void on_cancel() override;
  void on_complete() override;
  void on_draw_ui() override;
  void on_draw() override;

  bool on_flow_action(const std::string &action,
                      const interaction::flow_action_context &ctx) override;
  void on_handler_attached(const interaction::step_def &step,
                           interaction::input_handler *handler) override;

private:
  TopoDS_Face m_profile_face;
  glm::vec3 m_direction{0.0f, 0.0f, 1.0f};
  glm::vec3 m_anchor{0.0f};
  float m_preview_depth = 0.0f;
  float m_final_depth = 0.0f;
  handle m_extrude_handle{};
};

} // namespace toy_cad
