#pragma once

#include "tests/cad/interaction/flow_command.h"

#include <glm/glm.hpp>
#include <gp_Pnt.hxx>

namespace toy_cad {

class sketch_circle_command : public interaction::flow_command {
public:
  sketch_circle_command();

  void on_activate() override;
  void on_cancel() override;
  void on_draw_ui() override;

  bool on_flow_action(const std::string &action,
                      const interaction::flow_action_context &ctx) override;

private:
  gp_Pnt m_center{};
  gp_Pnt m_radius_point{};
  double m_preview_radius = 0.0;
  bool m_has_preview = false;
  glm::vec3 m_cursor_preview{0.0f};

  void sync_live_preview();
  static gp_Pnt from_glm(const glm::vec3 &v);
  static double distance_on_plane(const gp_Pnt &a, const gp_Pnt &b);
};

} // namespace toy_cad
