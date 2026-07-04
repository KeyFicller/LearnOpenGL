#include "history_tree.h"

#include "tests/cad/history/coordinate.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/inspector_tree_node.h"
#include "tests/cad/renderer/drawable_object.h"

#include "imgui.h"

#include <cstdio>

namespace toy_cad {

void history_tree::draw_local() {
  database &db = instance::get().db();
  for (handle h : m_handles) {
    auto *p = db.try_get_as<drawable_object>(h);
    if (p != nullptr) {
      p->draw_local();
    }
  }
}

void history_tree::draw_global() {
  database &db = instance::get().db();
  for (handle h : m_handles) {
    auto *p = db.try_get_as<drawable_object>(h);
    if (p != nullptr) {
      p->draw_global();
    }
  }
}

void history_tree::draw_ui() {
  database &db = instance::get().db();
  if (ImGui::Begin("Toy CAD History")) {
    const auto hist =
        interaction::tree_branch("History", ImGuiTreeNodeFlags_DefaultOpen);
    {
      interaction::tree_item_context_menu menu;
      if (menu) {
        ImGui::TextUnformatted("History root");
      }
    }
    if (hist.open) {
      const interaction::tree_indent_scope indent;
      int i = 0;
      for (handle h : m_handles) {
        ImGui::PushID(i);
        if (auto *obj = db.try_get_as<drawable_object>(h)) {
          obj->draw_ui(h);
        }
        ImGui::PopID();
        ++i;
      }
    }
  }
  ImGui::End();
}

} // namespace toy_cad
