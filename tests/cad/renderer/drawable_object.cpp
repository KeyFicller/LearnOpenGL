#include "drawable_object.h"

#include "imgui.h"

namespace toy_cad {

void drawable_object::inspect() const {
  object::inspect();
  ImGui::SeparatorText("drawable_object");
}

} // namespace toy_cad
