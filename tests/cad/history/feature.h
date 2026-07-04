#pragma once

#include "history_object.h"

namespace toy_cad {

/** CAD feature step in history: default {@link regen}/{@link rollback} no-op unless overridden. */
class feature : public history_object {
public:
  feature(const feature &) = delete;
  feature &operator=(const feature &) = delete;
  feature(feature &&) noexcept = default;
  feature &operator=(feature &&) noexcept = default;

  void regen() override;
  void rollback() override;
  void inspect() const override;

protected:
  feature() = default;
};

} // namespace toy_cad
