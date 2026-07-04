#pragma once

#include "feature.h"
#include "tests/cad/database/handle.h"

namespace toy_cad {

/** Solid / mesh-backed feature base (e.g. box, cylinder). */
class primitive_feature : public feature {
public:
  primitive_feature(const primitive_feature &) = delete;
  primitive_feature &operator=(const primitive_feature &) = delete;
  primitive_feature(primitive_feature &&) noexcept = default;
  primitive_feature &operator=(primitive_feature &&) noexcept = default;

  void inspect() const override;

  [[nodiscard]] virtual handle brep_handle() const = 0;
  [[nodiscard]] virtual bool has_brep() const { return brep_handle().valid(); }

protected:
  primitive_feature() = default;
};

} // namespace toy_cad
