#pragma once

#include "tests/cad/interaction/command_dispatcher.h"
#include "tests/cad/interaction/face_input_handler.h"
#include "tests/cad/interaction/extrude_depth_input_handler.h"
#include "tests/cad/database/handle.h"
#include <glm/glm.hpp>
#include <memory>

#include <TopoDS_Face.hxx>

namespace toy_cad {

class extrude_feature;

/**
 * Interactive command: extrude a picked face by dragging to set height.
 *
 * Steps:
 *   1. Pick a face (face_input_handler)
 *   2. Drag mouse to set extrusion depth along face normal (extrude_depth_input_handler)
 */
class extrude_command : public interaction::command {
public:
  extrude_command() = default;

  [[nodiscard]] const char *name() const override { return "Extrude"; }

  void on_activate() override;
  void on_cancel() override;
  void on_complete() override;

  void on_draw_ui() override;
  void on_draw() override;

  void on_input_changed(const char *flag, interaction::input_handler *handler,
                        bool confirmed) override;

private:
  enum class step { pick_face, pick_depth, done };

  step m_step = step::pick_face;

  TopoDS_Face m_profile_face;
  glm::vec3 m_direction{0.0f, 0.0f, 1.0f};
  glm::vec3 m_anchor{0.0f, 0.0f, 0.0f};
  float m_preview_depth = 0.0f;
  float m_final_depth = 0.0f;

  handle m_extrude_handle{};

  std::unique_ptr<interaction::face_input_handler> m_face_handler;
  std::unique_ptr<interaction::extrude_depth_input_handler> m_depth_handler;

  void setup_face_handler();
  void setup_depth_handler();
  void on_face_confirmed(handle face_h, const glm::vec3 &pick_point);
  void on_depth_confirmed(float depth);
  void update_preview_depth(float depth);
};

} // namespace toy_cad
