#include "primitive_feature.h"

#include "imgui.h"

namespace toy_cad {

void primitive_feature::inspect() const {
  feature::inspect();
  ImGui::SeparatorText("primitive_feature");
}

} // namespace toy_cad
