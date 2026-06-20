#include "brep_shell.h"

#include "imgui.h"

namespace toy_cad {

brep_shell::brep_shell(handle owner_brep) : m_owner_brep(owner_brep) {}

void brep_shell::add_face(handle h) { m_face_handles.push_back(h); }

void brep_shell::add_edge(handle h) { m_edge_handles.push_back(h); }

void brep_shell::clear_children() {
  m_face_handles.clear();
  m_edge_handles.clear();
}

void brep_shell::inspect() const {
  object::inspect();
  ImGui::SeparatorText("brep_shell");
  ImGui::BulletText("Faces: %zu", m_face_handles.size());
  ImGui::BulletText("Edges: %zu", m_edge_handles.size());
}

} // namespace toy_cad
