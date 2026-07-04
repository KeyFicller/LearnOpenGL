#include "extrude_command.h"

#include "extrude_feature.h"
#include "tests/cad/database/brep/brep_face.h"
#include "tests/cad/database/brep/brep_query.h"
#include "tests/cad/geometry/occt_mesh_utils.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/command_dispatcher.h"
#include "tests/cad/interaction/extrude_depth_input_handler.h"
#include "tests/cad/interaction/selection_manager.h"

#include "imgui.h"

#include <cmath>
#include <cstring>

namespace toy_cad {

void extrude_command::on_activate() {
  m_step = step::pick_face;
  m_profile_face = TopoDS_Face();
  m_direction = glm::vec3(0.0f, 0.0f, 1.0f);
  m_anchor = glm::vec3(0.0f);
  m_preview_depth = 0.0f;
  m_final_depth = 0.0f;
  m_extrude_handle = {};

  setup_face_handler();
}

void extrude_command::on_cancel() {
  if (m_extrude_handle.valid()) {
    instance::get().db().destroy(m_extrude_handle);
    m_extrude_handle = {};
  }
  interaction::selection_manager::instance().clear_hover_target();
}

void extrude_command::on_complete() {
  m_extrude_handle = {};
  interaction::selection_manager::instance().clear_hover_target();
}

void extrude_command::on_draw_ui() {
  ImGui::SeparatorText("Extrude");

  switch (m_step) {
  case step::pick_face:
    ImGui::Text("Step 1: Click a face to extrude");
    break;
  case step::pick_depth:
    ImGui::Text("Step 2: Move mouse to set height, click to confirm");
    ImGui::Text("Direction: (%.3f, %.3f, %.3f)", m_direction.x, m_direction.y,
                m_direction.z);
    ImGui::Text("Preview depth: %.3f", m_preview_depth);
    break;
  case step::done:
    ImGui::Text("Extrusion created (depth %.3f)", m_final_depth);
    break;
  }

  if (ImGui::Button("Cancel")) {
    interaction::command_dispatcher::instance().pop_command();
  }
}

void extrude_command::on_draw() {
  if (m_step != step::pick_depth || !m_extrude_handle.valid()) {
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

void extrude_command::on_input_changed(const char *flag,
                                       interaction::input_handler *handler,
                                       bool confirmed) {
  if (std::strcmp(flag, "face") == 0) {
    auto *face_handler =
        dynamic_cast<interaction::face_input_handler *>(handler);
    if (!face_handler || !face_handler->has_valid_hover()) {
      return;
    }

    if (confirmed && m_step == step::pick_face) {
      on_face_confirmed(*face_handler->hover_face(),
                        *face_handler->hover_point());
    }
    return;
  }

  if (std::strcmp(flag, "depth") == 0) {
    auto *depth_handler =
        dynamic_cast<interaction::extrude_depth_input_handler *>(handler);
    if (!depth_handler || !depth_handler->has_valid_preview()) {
      return;
    }

    if (m_step == step::pick_depth) {
      if (confirmed) {
        on_depth_confirmed(depth_handler->preview_depth());
      } else {
        update_preview_depth(depth_handler->preview_depth());
      }
    }
  }
}

void extrude_command::setup_face_handler() {
  m_face_handler = std::make_unique<interaction::face_input_handler>();
  m_face_handler->set_owner(this, "face");
  instance::get().push_input_handler(m_face_handler.get());
}

void extrude_command::setup_depth_handler() {
  m_depth_handler =
      std::make_unique<interaction::extrude_depth_input_handler>();
  m_depth_handler->set_extrude_frame(m_anchor, m_direction);
  m_depth_handler->set_owner(this, "depth");
  instance::get().push_input_handler(m_depth_handler.get());
}

void extrude_command::on_face_confirmed(handle face_h,
                                        const glm::vec3 &pick_point) {
  auto &db = instance::get().db();
  const brep_face *face_obj = try_get_face(db, face_h);
  if (!face_obj) {
    return;
  }

  m_profile_face = face_obj->face();
  m_direction = geometry::face_outward_normal(m_profile_face);
  m_anchor = pick_point;

  m_extrude_handle = db.emplace<extrude_feature>();
  if (auto *extrude = db.try_get_as<extrude_feature>(m_extrude_handle)) {
    extrude->set_profile(m_profile_face, m_direction, m_anchor);
    extrude->set_depth(0.0f);
  }

  instance::get().pop_input_handler();
  m_face_handler.reset();

  m_step = step::pick_depth;
  setup_depth_handler();
  m_depth_handler->sync_from_viewport_cursor();
}

void extrude_command::update_preview_depth(float depth) {
  m_preview_depth = depth;
  if (!m_extrude_handle.valid()) {
    return;
  }
  if (auto *extrude =
          instance::get().db().try_get_as<extrude_feature>(m_extrude_handle)) {
    extrude->set_depth(depth);
  }
}

void extrude_command::on_depth_confirmed(float depth) {
  if (std::abs(depth) < 1e-6f) {
    return;
  }

  m_final_depth = depth;
  update_preview_depth(depth);

  if (m_extrude_handle.valid()) {
    if (auto *extrude = instance::get().db().try_get_as<extrude_feature>(
            m_extrude_handle)) {
      extrude->regen();
    }
    instance::get().history().push_back(m_extrude_handle);
  }

  instance::get().pop_input_handler();
  m_depth_handler.reset();

  m_step = step::done;
  interaction::command_dispatcher::instance().complete_current();
}

} // namespace toy_cad
