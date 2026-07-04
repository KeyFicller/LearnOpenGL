#include "inspector_panel.h"

#include "tests/cad/database/object.h"
#include "tests/cad/instance.h"

#include "imgui.h"

namespace toy_cad::interaction {

inspector_panel &inspector_panel::instance() {
  static inspector_panel inst;
  return inst;
}

void inspector_panel::set_target(handle h) { m_target = h; }

void inspector_panel::clear_target() { m_target = {}; }

void inspector_panel::draw_ui() {
  if (!ImGui::Begin("Toy CAD Inspector")) {
    ImGui::End();
    return;
  }

  ImGui::SeparatorText("Handle");
  ImGui::BulletText("valid: %s", m_target.valid() ? "yes" : "no");
  if (m_target.valid()) {
    ImGui::BulletText("index: %u", m_target.index);
    ImGui::BulletText("generation: %u", m_target.generation);
  }

  if (ImGui::Button("Clear target")) {
    clear_target();
  }

  ImGui::Separator();

  if (!m_target.valid()) {
    ImGui::TextDisabled("Right-click an explorer tree node and choose Inspect.");
  } else {
    database &db = instance::get().db();
    object *resolved = db.try_get(m_target);
    if (resolved == nullptr) {
      ImGui::TextColored(ImVec4(1.F, .4F, .2F, 1.F),
                         "Stale handle (object not alive).");
    } else {
      ImGui::PushID(static_cast<int>(m_target.index));
      resolved->inspect();
      ImGui::PopID();
    }
  }

  ImGui::End();
}

void append_tree_inspector_menu_items(handle h) {
  if (!h.valid()) {
    return;
  }
  if (ImGui::MenuItem("Inspect")) {
    inspector_panel::instance().set_target(h);
  }
}

} // namespace toy_cad::interaction
