#pragma once

#include "primitive_feature.h"

namespace toy_cad {

/** Axis-aligned box primitive (placeholder geometry / history hook). */
class box_feature : public primitive_feature {
public:
  box_feature() = default;

  void draw_global() override;
  void draw_local() override;
  void draw_ui(handle explorer_row) override;
  void inspect() const override;
};

} // namespace toy_cad
