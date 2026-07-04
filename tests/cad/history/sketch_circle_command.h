#pragma once

#include "tests/cad/interaction/command_dispatcher.h"
#include "tests/cad/interaction/sketch_point_input_handler.h"

#include <glm/glm.hpp>
#include <gp_Pnt.hxx>
#include <memory>

namespace toy_cad {

/** Draw a circle on the active sketch plane (center + radius point). */
class sketch_circle_command : public interaction::command {
public:
  sketch_circle_command() = default;

  [[nodiscard]] const char *name() const override { return "Sketch Circle"; }

  void on_activate() override;
  void on_cancel() override;

  bool on_update() override;

  void on_draw_ui() override;
  void on_draw() override;

  void on_input_changed(const char *flag, interaction::input_handler *handler,
                        bool confirmed) override;

private:
  enum class step { pick_center, pick_radius, done };

  step m_step = step::pick_center;
  gp_Pnt m_center{};
  gp_Pnt m_radius_point{};
  double m_preview_radius = 0.0;
  bool m_has_preview = false;
  glm::vec3 m_cursor_preview{0.0f};

  std::unique_ptr<interaction::sketch_point_input_handler> m_center_handler;
  std::unique_ptr<interaction::sketch_point_input_handler> m_radius_handler;

  enum class deferred_action { none, advance_to_radius, complete };
  deferred_action m_deferred = deferred_action::none;

  void setup_center_handler();
  void setup_radius_handler();
  void on_center_confirmed(const gp_Pnt &point);
  void on_radius_confirmed(const gp_Pnt &point);
  void sync_live_preview();
  static gp_Pnt from_glm(const glm::vec3 &v);
  static double distance_on_plane(const gp_Pnt &a, const gp_Pnt &b);
};

} // namespace toy_cad
