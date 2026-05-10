#include "history_object.h"

#include "imgui.h"

namespace toy_cad {

void history_object::inspect() const {
  drawable_object::inspect();
  ImGui::SeparatorText("history_object");
}

} // namespace toy_cad
