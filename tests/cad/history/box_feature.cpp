#include "box_feature.h"

#include "imgui.h"

namespace toy_cad {

void box_feature::draw_global() {}

void box_feature::draw_local() {}

void box_feature::draw_ui(handle explorer_row) {
  (void)explorer_row;
}

void box_feature::inspect() const {
  primitive_feature::inspect();
  ImGui::SeparatorText("box_feature");
}

} // namespace toy_cad
