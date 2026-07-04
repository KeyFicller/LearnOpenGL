#include "sketch_circle_command.h"

#include "sketch_feature.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/command_dispatcher.h"
#include "tests/cad/interaction/sketch_context.h"

#include "imgui.h"

#include <cmath>
#include <cstring>

namespace toy_cad {

gp_Pnt sketch_circle_command::from_glm(const glm::vec3 &v) {
  return gp_Pnt(v.x, v.y, v.z);
}

double sketch_circle_command::distance_on_plane(const gp_Pnt &a,
                                                const gp_Pnt &b) {
  return a.Distance(b);
}

void sketch_circle_command::on_activate() {
  m_step = step::pick_center;
  m_center = gp_Pnt();
  m_radius_point = gp_Pnt();
  m_preview_radius = 0.0;
  m_has_preview = false;
  m_cursor_preview = glm::vec3(0.0f);
  m_deferred = deferred_action::none;
  sketch_context::instance().clear_live_preview();
  setup_center_handler();
}

void sketch_circle_command::on_cancel() {
  if (m_center_handler) {
    instance::get().pop_input_handler();
    m_center_handler.reset();
  }
  if (m_radius_handler) {
    instance::get().pop_input_handler();
    m_radius_handler.reset();
  }
  sketch_context::instance().clear_live_preview();
}

void sketch_circle_command::sync_live_preview() {
  sketch_live_preview preview;
  if (m_has_preview) {
    preview.show_cursor = true;
    preview.cursor_world = m_cursor_preview;
  }
  if (m_step == step::pick_radius && m_preview_radius > 1e-6) {
    preview.show_circle = true;
    preview.circle_center = m_center;
    preview.circle_radius = m_preview_radius;
  }
  sketch_context::instance().set_live_preview(preview);
}

void sketch_circle_command::on_draw_ui() {
  ImGui::SeparatorText("Sketch Circle");

  switch (m_step) {
  case step::pick_center:
    ImGui::Text("Click circle center");
    break;
  case step::pick_radius:
    ImGui::Text("Click radius point");
    ImGui::Text("Preview radius: %.3f", m_preview_radius);
    break;
  case step::done:
    ImGui::Text("Circle added");
    break;
  }

  if (ImGui::Button("Cancel")) {
    interaction::command_dispatcher::instance().pop_command();
  }
}

void sketch_circle_command::on_draw() {
  (void)m_step;
}

bool sketch_circle_command::on_update() {
  if (m_deferred == deferred_action::none) {
    return true;
  }

  const deferred_action action = m_deferred;
  m_deferred = deferred_action::none;

  if (action == deferred_action::advance_to_radius) {
    if (m_center_handler) {
      instance::get().pop_input_handler();
      m_center_handler.reset();
    }
    m_step = step::pick_radius;
    setup_radius_handler();
    sync_live_preview();
    return true;
  }

  if (action == deferred_action::complete) {
    if (m_radius_handler) {
      instance::get().pop_input_handler();
      m_radius_handler.reset();
    }
    sketch_context::instance().clear_live_preview();
    m_step = step::done;
    return false;
  }

  return true;
}

void sketch_circle_command::on_input_changed(
    const char *flag, interaction::input_handler *handler, bool confirmed) {
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

  if (std::strcmp(flag, "center") == 0) {
    if (confirmed) {
      if (m_step == step::pick_center) {
        on_center_confirmed(point);
        return;
      }
      if (m_step == step::pick_radius && m_center_handler) {
        on_radius_confirmed(point);
        return;
      }
    }
  } else if (std::strcmp(flag, "radius") == 0) {
    if (m_step == step::pick_radius) {
      if (confirmed) {
        on_radius_confirmed(point);
        return;
      }
      m_radius_point = point;
      m_preview_radius = distance_on_plane(m_center, m_radius_point);
    }
  }

  sync_live_preview();
}

void sketch_circle_command::setup_center_handler() {
  m_center_handler =
      std::make_unique<interaction::sketch_point_input_handler>();
  m_center_handler->set_owner(this, "center");
  instance::get().push_input_handler(m_center_handler.get());
  m_center_handler->sync_from_viewport_cursor();
}

void sketch_circle_command::setup_radius_handler() {
  m_radius_handler = std::make_unique<interaction::sketch_point_input_handler>();
  m_radius_handler->set_owner(this, "radius");
  instance::get().push_input_handler(m_radius_handler.get());
  m_radius_handler->sync_from_viewport_cursor();
}

void sketch_circle_command::on_center_confirmed(const gp_Pnt &point) {
  m_center = point;
  m_radius_point = point;
  m_preview_radius = 0.0;
  m_cursor_preview = glm::vec3(static_cast<float>(point.X()),
                               static_cast<float>(point.Y()),
                               static_cast<float>(point.Z()));
  m_step = step::pick_radius;
  m_deferred = deferred_action::advance_to_radius;
  sync_live_preview();
}

void sketch_circle_command::on_radius_confirmed(const gp_Pnt &point) {
  m_radius_point = point;
  const double radius = distance_on_plane(m_center, m_radius_point);
  if (radius < 1e-6) {
    return;
  }

  if (sketch_feature *sketch = sketch_context::instance().sketch()) {
    sketch->add_circle(m_center, radius);
  }

  m_deferred = deferred_action::complete;
}

} // namespace toy_cad
