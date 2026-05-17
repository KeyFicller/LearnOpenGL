#include "datum.h"

#include "tests/cad/instance.h"
#include "tests/cad/interaction/doc_input_handler.h"
#include "tests/cad/interaction/inspector.h"
#include "tests/cad/interaction/inspector_tree_node.h"
#include "tests/cad/renderer/viewport_axes_gizmo.h"

#include "basic/shader.h"

#include "imgui.h"

#include <glm/glm.hpp>

namespace toy_cad {

void datum::draw_global() {}

void datum::draw_local() {
  instance::get().viewport_shader().use();
  auto &disp = instance::get().disp();
  const glm::mat4 clip = disp.clip_from_world();
  const glm::mat4 view = disp.view_matrix;
  const gp_Pnt o = m_frame.Location();
  const glm::vec3 anchor(static_cast<float>(o.X()), static_cast<float>(o.Y()),
                         static_cast<float>(o.Z()));
  viewport_axes_gizmo::draw_datum_planes_screen_fixed(anchor, view, clip);
}

void datum::draw_explorer_leaf(const char *leaf_label, handle row) {
  auto &doc = interaction::doc_input_handler::instance();
  [[maybe_unused]] const auto leaf =
      interaction::tree_leaf(leaf_label, 0, row, doc);
  interaction::tree_item_context_menu menu;
  if (menu) {
    interaction::append_tree_inspector_menu_items(row);
    ImGui::TextUnformatted("datum");
  }
}

void datum::inspect() const {
  drawable_object::inspect();
  ImGui::SeparatorText("datum (gp_Ax2)");
  const gp_Pnt o = m_frame.Location();
  const gp_Dir xd = m_frame.XDirection();
  const gp_Dir yd = m_frame.YDirection();
  const gp_Dir zd = m_frame.Direction();
  ImGui::BulletText("Origin  (%.6g, %.6g, %.6g)", o.X(), o.Y(), o.Z());
  ImGui::BulletText("XDirection (%.6g, %.6g, %.6g)", xd.X(), xd.Y(), xd.Z());
  ImGui::BulletText("YDirection (%.6g, %.6g, %.6g)", yd.X(), yd.Y(), yd.Z());
  ImGui::BulletText("Axis direction / Z (%.6g, %.6g, %.6g)", zd.X(), zd.Y(),
                    zd.Z());
}

} // namespace toy_cad
