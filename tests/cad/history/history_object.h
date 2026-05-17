#pragma once

#include "tests/cad/renderer/drawable_object.h"

namespace toy_cad {

class history_object : public drawable_object {
public:
  history_object(const history_object &) = delete;
  history_object &operator=(const history_object &) = delete;
  history_object(history_object &&) noexcept = default;
  history_object &operator=(history_object &&) noexcept = default;

  virtual void regen() = 0;
  virtual void rollback() = 0;

  void inspect() const override;

protected:
  history_object() = default;
};

} // namespace toy_cad
