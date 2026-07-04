#pragma once

#include "tests/cad/interaction/command_dispatcher.h"
#include "tests/cad/interaction/sketch_point_input_handler.h"

#include <glm/glm.hpp>
#include <gp_Pnt.hxx>
#include <memory>

namespace toy_cad {

/** Draw a line segment on the active sketch plane. */
class sketch_line_command : public interaction::command {
public:
  sketch_line_command() = default;

  [[nodiscard]] const char *name() const override { return "Sketch Line"; }

  void on_activate() override;
  void on_cancel() override;

  bool on_update() override;

  void on_draw_ui() override;
  void on_draw() override;

  void on_input_changed(const char *flag, interaction::input_handler *handler,
                        bool confirmed) override;

private:
  enum class step { pick_p1, pick_p2, done };

  step m_step = step::pick_p1;
  gp_Pnt m_p1{};
  gp_Pnt m_p2{};
  gp_Pnt m_preview_p2{};
  bool m_has_preview = false;
  glm::vec3 m_cursor_preview{0.0f};

  std::unique_ptr<interaction::sketch_point_input_handler> m_p1_handler;
  std::unique_ptr<interaction::sketch_point_input_handler> m_p2_handler;

  enum class deferred_action { none, advance_to_p2, complete };
  deferred_action m_deferred = deferred_action::none;

  void setup_p1_handler();
  void setup_p2_handler();
  void on_p1_confirmed(const gp_Pnt &point);
  void on_p2_confirmed(const gp_Pnt &point);
  void sync_live_preview();
  static gp_Pnt from_glm(const glm::vec3 &v);
};

} // namespace toy_cad
