#pragma once

#include "tests/cad/renderer/drawable_object.h"
#include <gp_Ax1.hxx>

namespace toy_cad {

class axis : public drawable_object {
public:
  axis() = default;
  explicit axis(const gp_Ax1 &ax) : m_ax(ax) {}

  [[nodiscard]] gp_Ax1 &ax1() { return m_ax; }
  [[nodiscard]] const gp_Ax1 &ax1() const { return m_ax; }

  void set_ax1(const gp_Ax1 &ax) { m_ax = ax; }

  void draw_global() override;
  void draw_local() override;
  void draw_ui(handle explorer_row) override;

  void inspect() const override;

private:
  gp_Ax1 m_ax{};
};

} // namespace toy_cad
