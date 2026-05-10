#pragma once

#include <cstddef>
#include <cstdint>

namespace toy_cad {

struct handle {
  uint32_t index = 0xffffffffu;
  uint32_t generation = 0;

  [[nodiscard]] bool valid() const { return index != 0xffffffffu; }

  friend bool operator==(handle a, handle b) {
    return a.index == b.index && a.generation == b.generation;
  }
  friend bool operator!=(handle a, handle b) { return !(a == b); }
};

struct handle_hash {
  [[nodiscard]] std::size_t operator()(handle h) const noexcept {
    return static_cast<std::size_t>(h.index) ^
           (static_cast<std::size_t>(h.generation) << 16u);
  }
};

} // namespace toy_cad
