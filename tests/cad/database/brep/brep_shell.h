#pragma once

#include "tests/cad/database/handle.h"
#include "tests/cad/database/object.h"

#include <vector>

namespace toy_cad {

class brep_shell final : public object {
public:
  brep_shell() = default;
  explicit brep_shell(handle owner_brep);

  [[nodiscard]] handle owner_brep() const { return m_owner_brep; }
  [[nodiscard]] const std::vector<handle> &face_handles() const {
    return m_face_handles;
  }
  [[nodiscard]] const std::vector<handle> &edge_handles() const {
    return m_edge_handles;
  }

  void add_face(handle h);
  void add_edge(handle h);
  void clear_children();

  void inspect() const override;

private:
  handle m_owner_brep;
  std::vector<handle> m_face_handles;
  std::vector<handle> m_edge_handles;
};

} // namespace toy_cad
