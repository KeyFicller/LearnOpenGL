#include "extrude_command.h"

#include "extrude_feature.h"
#include "tests/cad/database/brep/brep_face.h"
#include "tests/cad/database/brep/brep_query.h"
#include "tests/cad/geometry/occt_mesh_utils.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/extrude_depth_input_handler.h"
#include "tests/cad/interaction/face_input_handler.h"
#include "tests/cad/interaction/selection_manager.h"

#include "imgui.h"

#include <cmath>

namespace toy_cad {

extrude_command::extrude_command() : flow_command("extrude") {}

void extrude_command::on_activate() {
  m_profile_face = TopoDS_Face();
  m_direction = glm::vec3(0.0f, 0.0f, 1.0f);
  m_anchor = glm::vec3(0.0f);
  m_preview_depth = 0.0f;
  m_final_depth = 0.0f;
  m_extrude_handle = {};
  flow_command::on_activate();
}

void extrude_command::on_cancel() {
  if (m_extrude_handle.valid()) {
    instance::get().db().destroy(m_extrude_handle);
    m_extrude_handle = {};
  }
  interaction::selection_manager::instance().clear_hover_target();
  flow_command::on_cancel();
}

void extrude_command::on_complete() {
  m_extrude_handle = {};
  interaction::selection_manager::instance().clear_hover_target();
  flow_command::on_complete();
}

void extrude_command::on_draw_ui() {
  flow_command::on_draw_ui();

  if (current_step_id() == "pick_depth") {
    ImGui::Text("Direction: (%.3f, %.3f, %.3f)", m_direction.x, m_direction.y,
                m_direction.z);
    ImGui::Text("Preview depth: %.3f", m_preview_depth);
  } else if (current_step_id() == "done") {
    ImGui::Text("Extrusion created (depth %.3f)", m_final_depth);
  }
}

void extrude_command::on_draw() {
  if (current_step_id() != "pick_depth" || !m_extrude_handle.valid()) {
    return;
  }

  if (auto *extrude =
          instance::get().db().try_get_as<extrude_feature>(m_extrude_handle)) {
    if (std::abs(m_preview_depth) >= 1e-6f) {
      extrude->regen();
      extrude->draw_global();
    }
  }
}

bool extrude_command::on_flow_action(const std::string &action,
                                      const interaction::flow_action_context &ctx) {
  if (action == "extrude.on_face_confirmed") {
    auto *face_handler =
        dynamic_cast<interaction::face_input_handler *>(ctx.handler);
    if (!face_handler || !face_handler->has_valid_hover()) {
      return true;
    }

    auto &db = instance::get().db();
    const handle face_h = *face_handler->hover_face();
    const brep_face *face_obj = try_get_face(db, face_h);
    if (!face_obj) {
      return true;
    }

    m_profile_face = face_obj->face();
    m_direction = geometry::face_outward_normal(m_profile_face);
    m_anchor = *face_handler->hover_point();

    m_extrude_handle = db.emplace<extrude_feature>();
    if (auto *extrude = db.try_get_as<extrude_feature>(m_extrude_handle)) {
      extrude->set_profile(m_profile_face, m_direction, m_anchor);
      extrude->set_depth(0.0f);
    }
    return false;
  }

  if (action == "extrude.on_depth_preview") {
    auto *depth_handler =
        dynamic_cast<interaction::extrude_depth_input_handler *>(ctx.handler);
    if (!depth_handler || !depth_handler->has_valid_preview()) {
      return false;
    }
    m_preview_depth = depth_handler->preview_depth();
    if (m_extrude_handle.valid()) {
      if (auto *extrude = instance::get().db().try_get_as<extrude_feature>(
              m_extrude_handle)) {
        extrude->set_depth(m_preview_depth);
      }
    }
    return false;
  }

  if (action == "extrude.on_depth_confirmed") {
    auto *depth_handler =
        dynamic_cast<interaction::extrude_depth_input_handler *>(ctx.handler);
    if (!depth_handler || !depth_handler->has_valid_preview()) {
      return true;
    }
    const float depth = depth_handler->preview_depth();
    if (std::abs(depth) < 1e-6f) {
      return true;
    }
    m_final_depth = depth;
    m_preview_depth = depth;
    if (m_extrude_handle.valid()) {
      if (auto *extrude = instance::get().db().try_get_as<extrude_feature>(
              m_extrude_handle)) {
        extrude->set_depth(depth);
        extrude->regen();
      }
      instance::get().history().push_back(m_extrude_handle);
    }
    return false;
  }

  return false;
}

void extrude_command::on_handler_attached(const interaction::step_def &step,
                                          interaction::input_handler *handler) {
  if (step.id != "pick_depth") {
    return;
  }
  if (auto *depth =
          dynamic_cast<interaction::extrude_depth_input_handler *>(handler)) {
    depth->set_extrude_frame(m_anchor, m_direction);
  }
}

} // namespace toy_cad
