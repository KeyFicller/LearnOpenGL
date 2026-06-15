#pragma once

#include "feature.h"

namespace toy_cad {

/** Solid / mesh-backed feature base (e.g. box, cylinder). */
class primitive_feature : public feature {
public:
  primitive_feature(const primitive_feature &) = delete;
  primitive_feature &operator=(const primitive_feature &) = delete;
  primitive_feature(primitive_feature &&) noexcept = default;
  primitive_feature &operator=(primitive_feature &&) noexcept = default;

  void inspect() const override;

protected:
  primitive_feature() = default;
};

} // namespace toy_cad
