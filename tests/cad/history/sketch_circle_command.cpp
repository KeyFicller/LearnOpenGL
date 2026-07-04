#include "sketch_circle_command.h"

#include "sketch_feature.h"
#include "tests/cad/interaction/sketch_context.h"
#include "tests/cad/interaction/sketch_point_input_handler.h"

#include "imgui.h"

namespace toy_cad {

gp_Pnt sketch_circle_command::from_glm(const glm::vec3 &v) {
  return gp_Pnt(v.x, v.y, v.z);
}

double sketch_circle_command::distance_on_plane(const gp_Pnt &a,
                                                const gp_Pnt &b) {
  return a.Distance(b);
}

sketch_circle_command::sketch_circle_command()
    : flow_command("sketch.circle") {}

void sketch_circle_command::on_activate() {
  m_center = gp_Pnt();
  m_radius_point = gp_Pnt();
  m_preview_radius = 0.0;
  m_has_preview = false;
  m_cursor_preview = glm::vec3(0.0f);
  sketch_context::instance().clear_live_preview();
  flow_command::on_activate();
}

void sketch_circle_command::on_cancel() {
  sketch_context::instance().clear_live_preview();
  flow_command::on_cancel();
}

void sketch_circle_command::sync_live_preview() {
  sketch_live_preview preview;
  if (m_has_preview) {
    preview.show_cursor = true;
    preview.cursor_world = m_cursor_preview;
  }
  if (current_step_id() == "pick_radius" && m_preview_radius > 1e-6) {
    preview.show_circle = true;
    preview.circle_center = m_center;
    preview.circle_radius = m_preview_radius;
  }
  sketch_context::instance().set_live_preview(preview);
}

void sketch_circle_command::on_draw_ui() {
  flow_command::on_draw_ui();
  if (current_step_id() == "pick_radius") {
    ImGui::Text("Preview radius: %.3f", m_preview_radius);
  }
}

bool sketch_circle_command::on_flow_action(
    const std::string &action, const interaction::flow_action_context &ctx) {
  auto *point_handler =
      dynamic_cast<interaction::sketch_point_input_handler *>(ctx.handler);

  if (action == "sketch_circle.on_preview") {
    if (!point_handler || !point_handler->has_valid_preview()) {
      m_has_preview = false;
      sync_live_preview();
      return false;
    }
    m_cursor_preview = point_handler->preview_point();
    m_has_preview = true;
    if (current_step_id() == "pick_radius") {
      m_radius_point = from_glm(m_cursor_preview);
      m_preview_radius = distance_on_plane(m_center, m_radius_point);
    }
    sync_live_preview();
    return false;
  }

  if (action == "sketch_circle.on_center_confirmed") {
    if (!point_handler || !point_handler->has_valid_preview()) {
      return true;
    }
    m_center = from_glm(point_handler->preview_point());
    m_radius_point = m_center;
    m_preview_radius = 0.0;
    m_cursor_preview = point_handler->preview_point();
    m_has_preview = true;
    sync_live_preview();
    return false;
  }

  if (action == "sketch_circle.on_radius_confirmed") {
    if (!point_handler || !point_handler->has_valid_preview()) {
      return true;
    }
    m_radius_point = from_glm(point_handler->preview_point());
    const double radius = distance_on_plane(m_center, m_radius_point);
    if (radius < 1e-6) {
      return true;
    }
    if (sketch_feature *sketch = sketch_context::instance().sketch()) {
      sketch->add_circle(m_center, radius);
    }
    sketch_context::instance().clear_live_preview();
    return false;
  }

  return false;
}

} // namespace toy_cad
