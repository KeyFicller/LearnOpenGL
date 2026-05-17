#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <typeinfo>

#if defined(__GNUC__) || defined(__clang__)
#include <cstdlib>
#include <cxxabi.h>
#endif

namespace toy_cad {

namespace detail {

[[nodiscard]] inline std::string demangle_typename(const char *mangled) {
#if defined(__GNUC__) || defined(__clang__)
  int status = 0;
  char *dem = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
  std::string out = (status == 0 && dem) ? dem : mangled;
  std::free(dem);
  return out;
#else
  return mangled;
#endif
}

[[nodiscard]] inline std::string short_type_name(std::string s) {
  const auto cut = s.rfind("::");
  if (cut != std::string::npos && cut + 2 < s.size()) {
    return s.substr(cut + 2);
  }
  return s;
}

} // namespace detail

template <typename T>
[[nodiscard]] inline std::string type_name_short() {
  return detail::short_type_name(detail::demangle_typename(typeid(T).name()));
}

template <typename T>
struct name_generator {
  [[nodiscard]] static std::string next() {
    static std::atomic<uint32_t> seq{0};
    const uint32_t id =
        seq.fetch_add(1, std::memory_order_relaxed);
    return type_name_short<T>() + '_' + std::to_string(static_cast<unsigned long>(id));
  }
};

} // namespace toy_cad
