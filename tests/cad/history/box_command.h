#pragma once

#include "tests/cad/interaction/command_dispatcher.h"
#include "tests/cad/database/handle.h"
#include <glm/glm.hpp>
#include <memory>

namespace toy_cad {

class box_feature;

/**
 * Interactive command: create box by picking two diagonal corners.
 *
 * Step 1: Click to set corner point 1 (p1)
 * Step 2: Move mouse to preview box, click to set corner point 2 (p2)
 * Step 3: Command completes, box is created in history
 */
class box_command : public interaction::command {
public:
  box_command() = default;

  [[nodiscard]] const char *name() const override { return "Create Box"; }

  void on_activate() override;
  void on_cancel() override;
  void on_complete() override;

  void on_draw_ui() override;
  void on_draw() override;
  bool on_mouse_moved(double xpos, double ypos) override;
  bool on_mouse_button(int button, int action, int mods) override;

private:
  enum class step { pick_p1, pick_p2, done };

  step m_step = step::pick_p1;
  glm::vec3 m_p1{0.0f, 0.0f, 0.0f};
  glm::vec3 m_p2{0.0f, 0.0f, 0.0f};
  glm::vec3 m_preview_p2{0.0f, 0.0f, 0.0f}; // Current mouse pos on working plane

  handle m_box_handle{}; // Handle to the box_feature being created

  double m_mouse_x = 0.0; // Last mouse X for ray pick
  double m_mouse_y = 0.0; // Last mouse Y for ray pick

  void update_preview(double screen_x, double screen_y);
  void commit_box();
};

} // namespace toy_cad
