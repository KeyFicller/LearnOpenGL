#include "coordinate.h"

#include "tests/cad/database/axis.h"
#include "tests/cad/database/database.h"
#include "tests/cad/database/datum.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/doc_input_handler.h"
#include "tests/cad/document/inspector_panel.h"
#include "tests/cad/interaction/inspector_tree_node.h"

#include "glad/gl.h"
#include "imgui.h"

#include <cstdio>
#include <string>

namespace toy_cad {

coordinate::coordinate(std::array<handle, 3> datum_handles,
                       std::array<handle, 3> axis_handles)
    : m_datum_handles(datum_handles), m_axis_handles(axis_handles) {}

void coordinate::draw_global() {
  database &db = instance::get().db();
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);
  for (handle h : m_datum_handles) {
    if (auto *d = db.try_get_as<datum>(h)) {
      d->draw_global();
    }
  }
  glDisable(GL_BLEND);
  for (handle h : m_axis_handles) {
    if (auto *a = db.try_get_as<axis>(h)) {
      a->draw_global();
    }
  }
  glDepthMask(GL_TRUE);
}

void coordinate::draw_local() {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDepthMask(GL_FALSE);
  database &db = instance::get().db();
  for (handle h : m_datum_handles) {
    if (auto *d = db.try_get_as<datum>(h)) {
      d->draw_local();
    }
  }
  glDisable(GL_BLEND);
  for (handle h : m_axis_handles) {
    if (auto *a = db.try_get_as<axis>(h)) {
      a->draw_local();
    }
  }
  glDepthMask(GL_TRUE);
}

void coordinate::draw_ui(handle explorer_row) {
  auto &doc = interaction::doc_input_handler::instance();
  database &db = instance::get().db();
  std::string root = tag();
  if (root.empty()) {
    root = "Coordinate";
  }
  root += "###coordinate_root";
  const auto coord = interaction::tree_branch(
      root.c_str(), ImGuiTreeNodeFlags_DefaultOpen, explorer_row, doc);
  {
    interaction::tree_item_context_menu menu;
    if (menu) {
      interaction::append_tree_inspector_menu_items(explorer_row);
      ImGui::TextUnformatted(tag().empty() ? "Coordinate" : tag().c_str());
    }
  }
  if (!coord.open) {
    return;
  }

  const interaction::tree_indent_scope indent;

  for (int i = 0; i < 3; ++i) {
    ImGui::PushID(i);
    auto *d = db.try_get_as<datum>(m_datum_handles[static_cast<size_t>(i)]);
    if (d != nullptr) {
      std::string leaf = d->tag();
      if (leaf.empty()) {
        char fb[48];
        std::snprintf(fb, sizeof(fb), "基准面 %d", i);
        leaf = fb;
      }
      leaf += "###coord_datum_";
      leaf += std::to_string(i);
      d->draw_explorer_leaf(leaf.c_str(),
                            m_datum_handles[static_cast<size_t>(i)]);
    }
    ImGui::PopID();
  }

  for (int i = 0; i < 3; ++i) {
    ImGui::PushID(i + 3);
    auto *a = db.try_get_as<axis>(m_axis_handles[static_cast<size_t>(i)]);
    if (a != nullptr) {
      std::string leaf = a->tag();
      if (leaf.empty()) {
        char fb[48];
        std::snprintf(fb, sizeof(fb), "轴 %d", i);
        leaf = fb;
      }
      leaf += "###coord_axis_";
      leaf += std::to_string(i);
      a->draw_explorer_leaf(leaf.c_str(),
                            m_axis_handles[static_cast<size_t>(i)]);
    }
    ImGui::PopID();
  }
}

void coordinate::regen() {}

void coordinate::rollback() {}

void coordinate::inspect() const {
  feature::inspect();
  ImGui::SeparatorText("coordinate");
  for (int i = 0; i < 3; ++i) {
    const handle hd = m_datum_handles[static_cast<size_t>(i)];
    if (hd.valid()) {
      ImGui::BulletText("datum_handles[%d] index=%u gen=%u", i, hd.index,
                        hd.generation);
    }
  }
  for (int i = 0; i < 3; ++i) {
    const handle ha = m_axis_handles[static_cast<size_t>(i)];
    if (ha.valid()) {
      ImGui::BulletText("axis_handles[%d] index=%u gen=%u", i, ha.index,
                        ha.generation);
    }
  }
}

} // namespace toy_cad
