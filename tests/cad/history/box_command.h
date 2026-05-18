#pragma once

#include "tests/cad/interaction/command_dispatcher.h"
#include "tests/cad/interaction/point_input_handler.h"
#include "tests/cad/database/handle.h"
#include <glm/glm.hpp>
#include <memory>

namespace toy_cad {

class box_feature;

/**
 * Interactive command: create box by picking two diagonal corners.
 *
 * Uses point_input_handler for interaction:
 *   - First handler ("p1") for picking corner point 1
 *   - Second handler ("p2") for picking corner point 2
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

  /**
   * Called by point_input_handler when point input changes or confirms.
   * @param flag "p1" or "p2" identifying which handler
   * @param handler the point_input_handler (extract preview_point from it)
   * @param confirmed true if this is a click confirmation, false for preview
   */
  void on_input_changed(const char *flag, interaction::input_handler *handler, bool confirmed) override;

private:
  enum class step { pick_p1, pick_p2, done };

  step m_step = step::pick_p1;
  glm::vec3 m_p1{0.0f, 0.0f, 0.0f};
  glm::vec3 m_p2{0.0f, 0.0f, 0.0f};
  glm::vec3 m_preview_p2{0.0f, 0.0f, 0.0f}; // Current mouse pos on working plane

  handle m_box_handle{}; // Handle to the box_feature being created

  std::unique_ptr<interaction::point_input_handler> m_p1_handler;
  std::unique_ptr<interaction::point_input_handler> m_p2_handler;

  void setup_p1_handler();
  void setup_p2_handler();
  void on_p1_confirmed(const glm::vec3 &point);
  void on_p2_confirmed(const glm::vec3 &point);
};

} // namespace toy_cad
