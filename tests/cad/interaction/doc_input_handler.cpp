#include "doc_input_handler.h"

#include "imgui.h"

namespace toy_cad::interaction {

doc_input_handler &doc_input_handler::instance() {
  static doc_input_handler doc;
  return doc;
}

void doc_input_handler::clear_selection() { m_selection.clear(); }

bool doc_input_handler::is_selected(handle h) const {
  return m_selection.find(h) != m_selection.end();
}

void doc_input_handler::on_explorer_item_left_click(handle item) {
  if (!item.valid()) {
    return;
  }
  ImGuiIO &io = ImGui::GetIO();
  if (io.KeyCtrl) {
    if (is_selected(item)) {
      m_selection.erase(item);
    } else {
      m_selection.insert(item);
    }
  } else {
    m_selection.clear();
    m_selection.insert(item);
  }
}

} // namespace toy_cad::interaction
