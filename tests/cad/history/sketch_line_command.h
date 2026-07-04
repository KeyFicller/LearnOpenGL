#pragma once

#include "tests/cad/interaction/flow_command.h"

#include <glm/glm.hpp>
#include <gp_Pnt.hxx>

namespace toy_cad {

class sketch_line_command : public interaction::flow_command {
public:
  sketch_line_command();

  void on_activate() override;
  void on_cancel() override;
  void on_draw_ui() override;

  bool on_flow_action(const std::string &action,
                      const interaction::flow_action_context &ctx) override;

private:
  gp_Pnt m_p1{};
  gp_Pnt m_p2{};
  gp_Pnt m_preview_p2{};
  bool m_has_preview = false;
  glm::vec3 m_cursor_preview{0.0f};

  void sync_live_preview();
  static gp_Pnt from_glm(const glm::vec3 &v);
};

} // namespace toy_cad
