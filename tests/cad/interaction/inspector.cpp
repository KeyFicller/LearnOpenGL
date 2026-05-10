#include "inspector.h"

#include "tests/cad/database/object.h"
#include "tests/cad/instance.h"

#include "imgui.h"

namespace toy_cad::interaction {

inspector &inspector::instance() {
  static inspector inst;
  return inst;
}

void inspector::set_target(handle h) { m_target = h; }

void inspector::clear_target() { m_target = {}; }

void inspector::draw_ui() {
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
    inspector::instance().set_target(h);
  }
}

} // namespace toy_cad::interaction
