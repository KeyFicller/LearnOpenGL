#pragma once

#include "feature.h"
#include "tests/cad/database/handle.h"
#include <array>

namespace toy_cad {

class coordinate : public feature {
public:
  explicit coordinate(std::array<handle, 3> datum_handles,
                      std::array<handle, 3> axis_handles);

  [[nodiscard]] std::array<handle, 3> &datum_handles() { return m_datum_handles; }
  [[nodiscard]] const std::array<handle, 3> &datum_handles() const {
    return m_datum_handles;
  }

  [[nodiscard]] std::array<handle, 3> &axis_handles() { return m_axis_handles; }
  [[nodiscard]] const std::array<handle, 3> &axis_handles() const {
    return m_axis_handles;
  }

  void draw_global() override;
  void draw_local() override;
  void draw_ui(handle explorer_row) override;
  void regen() override;
  void rollback() override;
  void inspect() const override;

private:
  std::array<handle, 3> m_datum_handles{};
  std::array<handle, 3> m_axis_handles{};
};

} // namespace toy_cad
