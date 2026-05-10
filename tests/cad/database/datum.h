#pragma once

#include "tests/cad/renderer/drawable_object.h"
#include <gp_Ax2.hxx>
#include <gp_Pln.hxx>

namespace toy_cad {

class datum : public drawable_object {
public:
  datum() = default;
  explicit datum(const gp_Ax2 &frame) : m_frame(frame) {}

  [[nodiscard]] gp_Ax2 &ax2() { return m_frame; }
  [[nodiscard]] const gp_Ax2 &ax2() const { return m_frame; }

  [[nodiscard]] gp_Pln plane() const { return gp_Pln(m_frame); }

  void set_ax2(const gp_Ax2 &frame) { m_frame = frame; }

  void draw_global() override;
  void draw_local() override;
  /** ImGui leaf inside coordinate subtree (explicit label incl. ### id). */
  void draw_explorer_leaf(const char *leaf_label, handle row);

  void inspect() const override;

private:
  gp_Ax2 m_frame{};
};

} // namespace toy_cad
