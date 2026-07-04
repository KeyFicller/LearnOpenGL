#include "sketch_line_command.h"

#include "sketch_feature.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/command_dispatcher.h"
#include "tests/cad/interaction/sketch_context.h"

#include "imgui.h"

#include <cstring>

namespace toy_cad {

gp_Pnt sketch_line_command::from_glm(const glm::vec3 &v) {
  return gp_Pnt(v.x, v.y, v.z);
}

void sketch_line_command::on_activate() {
  m_step = step::pick_p1;
  m_p1 = gp_Pnt();
  m_p2 = gp_Pnt();
  m_preview_p2 = gp_Pnt();
  m_has_preview = false;
  m_cursor_preview = glm::vec3(0.0f);
  m_deferred = deferred_action::none;
  sketch_context::instance().clear_live_preview();
  setup_p1_handler();
}

void sketch_line_command::on_cancel() {
  if (m_p1_handler) {
    instance::get().pop_input_handler();
    m_p1_handler.reset();
  }
  if (m_p2_handler) {
    instance::get().pop_input_handler();
    m_p2_handler.reset();
  }
  sketch_context::instance().clear_live_preview();
}

void sketch_line_command::sync_live_preview() {
  sketch_live_preview preview;
  if (m_has_preview) {
    preview.show_cursor = true;
    preview.cursor_world = m_cursor_preview;
  }
  if (m_step == step::pick_p2 && m_has_preview) {
    preview.show_segment = true;
    preview.seg_p1 = m_p1;
    preview.seg_p2 = m_preview_p2;
  }
  sketch_context::instance().set_live_preview(preview);
}

void sketch_line_command::on_draw_ui() {
  ImGui::SeparatorText("Sketch Line");

  switch (m_step) {
  case step::pick_p1:
    ImGui::Text("Click start point");
    break;
  case step::pick_p2:
    ImGui::Text("Click end point");
    if (m_has_preview) {
      ImGui::Text("Start: (%.3f, %.3f, %.3f)", m_p1.X(), m_p1.Y(), m_p1.Z());
      ImGui::Text("Preview: (%.3f, %.3f, %.3f)", m_preview_p2.X(),
                  m_preview_p2.Y(), m_preview_p2.Z());
    }
    break;
  case step::done:
    ImGui::Text("Line added");
    break;
  }

  if (ImGui::Button("Cancel")) {
    interaction::command_dispatcher::instance().pop_command();
  }
}

void sketch_line_command::on_draw() {
  (void)m_step;
}

bool sketch_line_command::on_update() {
  if (m_deferred == deferred_action::none) {
    return true;
  }

  const deferred_action action = m_deferred;
  m_deferred = deferred_action::none;

  if (action == deferred_action::advance_to_p2) {
    if (m_p1_handler) {
      instance::get().pop_input_handler();
      m_p1_handler.reset();
    }
    m_step = step::pick_p2;
    setup_p2_handler();
    sync_live_preview();
    return true;
  }

  if (action == deferred_action::complete) {
    if (m_p2_handler) {
      instance::get().pop_input_handler();
      m_p2_handler.reset();
    }
    sketch_context::instance().clear_live_preview();
    m_step = step::done;
    return false;
  }

  return true;
}

void sketch_line_command::on_input_changed(const char *flag,
                                           interaction::input_handler *handler,
                                           bool confirmed) {
  auto *point_handler =
      dynamic_cast<interaction::sketch_point_input_handler *>(handler);
  if (!point_handler || !point_handler->has_valid_preview()) {
    m_has_preview = false;
    sync_live_preview();
    return;
  }

  m_cursor_preview = point_handler->preview_point();
  m_has_preview = true;
  const gp_Pnt point = from_glm(m_cursor_preview);

  if (std::strcmp(flag, "p1") == 0) {
    if (confirmed) {
      if (m_step == step::pick_p1) {
        on_p1_confirmed(point);
        return;
      }
      if (m_step == step::pick_p2 && m_p1_handler) {
        on_p2_confirmed(point);
        return;
      }
    }
  } else if (std::strcmp(flag, "p2") == 0) {
    if (m_step == step::pick_p2) {
      if (confirmed) {
        on_p2_confirmed(point);
        return;
      }
      m_preview_p2 = point;
    }
  }

  sync_live_preview();
}

void sketch_line_command::setup_p1_handler() {
  m_p1_handler = std::make_unique<interaction::sketch_point_input_handler>();
  m_p1_handler->set_owner(this, "p1");
  instance::get().push_input_handler(m_p1_handler.get());
  m_p1_handler->sync_from_viewport_cursor();
}

void sketch_line_command::setup_p2_handler() {
  m_p2_handler = std::make_unique<interaction::sketch_point_input_handler>();
  m_p2_handler->set_owner(this, "p2");
  instance::get().push_input_handler(m_p2_handler.get());
  m_p2_handler->sync_from_viewport_cursor();
}

void sketch_line_command::on_p1_confirmed(const gp_Pnt &point) {
  m_p1 = point;
  m_preview_p2 = point;
  m_cursor_preview = glm::vec3(static_cast<float>(point.X()),
                               static_cast<float>(point.Y()),
                               static_cast<float>(point.Z()));
  m_step = step::pick_p2;
  m_deferred = deferred_action::advance_to_p2;
  sync_live_preview();
}

void sketch_line_command::on_p2_confirmed(const gp_Pnt &point) {
  m_p2 = point;

  if (sketch_feature *sketch = sketch_context::instance().sketch()) {
    sketch->add_line(m_p1, m_p2);
  }

  m_deferred = deferred_action::complete;
}

} // namespace toy_cad
