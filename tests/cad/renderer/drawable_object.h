#pragma once

#include "tests/cad/database/handle.h"
#include "tests/cad/database/object.h"

namespace toy_cad {

class drawable_object : public object {
public:
  drawable_object(const drawable_object &) = delete;
  drawable_object &operator=(const drawable_object &) = delete;
  drawable_object(drawable_object &&) noexcept = default;
  drawable_object &operator=(drawable_object &&) noexcept = default;

  /** World-space CAD geometry pass. */
  virtual void draw_global() = 0;
  /** Screen-space / overlay (labels in viewport HUD), after world pass. */
  virtual void draw_local() {}
  /** Inspector / explorer; `explorer_row` binds this row to a DB handle when shown in the tree. */
  virtual void draw_ui(handle explorer_row = {});

  /** ImGui hierarchical debug / properties; default chains `object`. */
  void inspect() const override;

protected:
  drawable_object() = default;
};

inline void drawable_object::draw_ui(handle) {}

} // namespace toy_cad
