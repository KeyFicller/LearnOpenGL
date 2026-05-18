#include "axis.h"

#include "tests/cad/instance.h"
#include "tests/cad/interaction/doc_input_handler.h"
#include "tests/cad/document/inspector_panel.h"
#include "tests/cad/interaction/inspector_tree_node.h"
#include "tests/cad/renderer/viewport_axes_gizmo.h"

#include "basic/shader.h"

#include "imgui.h"

#include <glm/glm.hpp>

namespace toy_cad {

void axis::draw_global() { (void)0; }

void axis::draw_local() {
  instance::get().viewport_shader().use();
  auto &disp = instance::get().disp();
  const glm::mat4 clip = disp.clip_from_world();
  const glm::mat4 view = disp.view_matrix;
  const gp_Pnt o = m_ax.Location();
  const glm::vec3 anchor(static_cast<float>(o.X()), static_cast<float>(o.Y()),
                         static_cast<float>(o.Z()));
  viewport_axes_gizmo::draw_axes_screen_fixed(anchor, view, clip);
}

void axis::draw_ui(handle explorer_row) {
  auto &doc = interaction::doc_input_handler::instance();

  // Build leaf label from tag
  std::string leaf = tag();
  if (leaf.empty()) {
    leaf = "axis";
  }
  leaf += "###axis_" + std::to_string(explorer_row.index);

  [[maybe_unused]] const auto leaf_out =
      interaction::tree_leaf(leaf.c_str(), 0, explorer_row, doc);
  interaction::tree_item_context_menu menu;
  if (menu) {
    interaction::append_tree_inspector_menu_items(explorer_row);
    ImGui::TextUnformatted(tag().empty() ? "axis" : tag().c_str());
  }
}

void axis::inspect() const {
  drawable_object::inspect();
  ImGui::SeparatorText("axis (gp_Ax1)");
  const gp_Pnt o = m_ax.Location();
  const gp_Dir dir = m_ax.Direction();
  ImGui::BulletText("Origin (%.6g, %.6g, %.6g)", o.X(), o.Y(), o.Z());
  ImGui::BulletText("Direction (%.6g, %.6g, %.6g)", dir.X(), dir.Y(), dir.Z());
}

} // namespace toy_cad
