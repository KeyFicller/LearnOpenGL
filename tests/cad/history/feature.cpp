#include "feature.h"

#include "imgui.h"

namespace toy_cad {

void feature::regen() {}

void feature::rollback() {}

void feature::inspect() const {
  history_object::inspect();
  ImGui::SeparatorText("feature");
}

} // namespace toy_cad
