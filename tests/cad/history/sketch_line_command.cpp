#include "sketch_line_command.h"

#include "sketch_feature.h"
#include "tests/cad/interaction/sketch_context.h"
#include "tests/cad/interaction/sketch_point_input_handler.h"

#include "imgui.h"

namespace toy_cad {

gp_Pnt sketch_line_command::from_glm(const glm::vec3 &v) {
  return gp_Pnt(v.x, v.y, v.z);
}

sketch_line_command::sketch_line_command() : flow_command("sketch.line") {}

void sketch_line_command::on_activate() {
  m_p1 = gp_Pnt();
  m_p2 = gp_Pnt();
  m_preview_p2 = gp_Pnt();
  m_has_preview = false;
  m_cursor_preview = glm::vec3(0.0f);
  sketch_context::instance().clear_live_preview();
  flow_command::on_activate();
}

void sketch_line_command::on_cancel() {
  sketch_context::instance().clear_live_preview();
  flow_command::on_cancel();
}

void sketch_line_command::sync_live_preview() {
  sketch_live_preview preview;
  if (m_has_preview) {
    preview.show_cursor = true;
    preview.cursor_world = m_cursor_preview;
  }
  if (current_step_id() == "pick_p2" && m_has_preview) {
    preview.show_segment = true;
    preview.seg_p1 = m_p1;
    preview.seg_p2 = m_preview_p2;
  }
  sketch_context::instance().set_live_preview(preview);
}

void sketch_line_command::on_draw_ui() {
  flow_command::on_draw_ui();
  if (current_step_id() == "pick_p2" && m_has_preview) {
    ImGui::Text("Start: (%.3f, %.3f, %.3f)", m_p1.X(), m_p1.Y(), m_p1.Z());
    ImGui::Text("Preview: (%.3f, %.3f, %.3f)", m_preview_p2.X(),
                m_preview_p2.Y(), m_preview_p2.Z());
  }
}

bool sketch_line_command::on_flow_action(const std::string &action,
                                         const interaction::flow_action_context &ctx) {
  auto *point_handler =
      dynamic_cast<interaction::sketch_point_input_handler *>(ctx.handler);

  if (action == "sketch_line.on_preview") {
    if (!point_handler || !point_handler->has_valid_preview()) {
      m_has_preview = false;
      sync_live_preview();
      return false;
    }
    m_cursor_preview = point_handler->preview_point();
    m_has_preview = true;
    if (current_step_id() == "pick_p2") {
      m_preview_p2 = from_glm(m_cursor_preview);
    }
    sync_live_preview();
    return false;
  }

  if (action == "sketch_line.on_p1_confirmed") {
    if (!point_handler || !point_handler->has_valid_preview()) {
      return true;
    }
    m_p1 = from_glm(point_handler->preview_point());
    m_preview_p2 = m_p1;
    m_cursor_preview = point_handler->preview_point();
    m_has_preview = true;
    sync_live_preview();
    return false;
  }

  if (action == "sketch_line.on_p2_confirmed") {
    if (!point_handler || !point_handler->has_valid_preview()) {
      return true;
    }
    m_p2 = from_glm(point_handler->preview_point());
    if (sketch_feature *sketch = sketch_context::instance().sketch()) {
      sketch->add_line(m_p1, m_p2);
    }
    sketch_context::instance().clear_live_preview();
    return false;
  }

  return false;
}

} // namespace toy_cad
