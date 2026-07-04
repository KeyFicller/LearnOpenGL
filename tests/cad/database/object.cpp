#include "object.h"

#include "imgui.h"

namespace toy_cad {

void object::inspect() const {
  ImGui::SeparatorText("object");
  ImGui::BulletText("tag: %s", tag().empty() ? "<empty>" : tag().c_str());
}

} // namespace toy_cad
