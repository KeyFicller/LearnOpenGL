#pragma once

#include <string>
#include <vector>

struct script_namespace : public std::string {};
inline script_namespace operator""_ns(const char *_ns, size_t) {
  return script_namespace(_ns);
}

struct script_class : public std::string {};
inline script_class operator""_cls(const char *_c, size_t) {
  return script_class(_c);
}

struct script_method : public std::string {};
inline script_method operator""_md(const char *_m, size_t) {
  return script_method(_m);
}

namespace mono_invoker {

struct script_ncm {
  script_namespace ns;
  script_class cls;
  script_method md;

  script_ncm(const script_namespace &_ns, const script_class &_cls,
             const script_method &_md)
      : ns(_ns), cls(_cls), md(_md) {}
};

enum class invoke_result { k_integer, k_float, k_string };

template <typename T> struct invoke_result_helper;

template <> struct invoke_result_helper<int> {
  static constexpr invoke_result value = invoke_result::k_integer;
};

template <> struct invoke_result_helper<float> {
  static constexpr invoke_result value = invoke_result::k_float;
};

template <> struct invoke_result_helper<std::string> {
  static constexpr invoke_result value = invoke_result::k_string;
};

template <typename T>
inline constexpr invoke_result invoke_result_v = invoke_result_helper<T>::value;

class invoker_impl;

class invoker {
  friend class invoker_impl;

public:
  invoker();
  ~invoker();

public:
  bool is_ready() const;
  bool invoke(const script_ncm &ncm, std::string &result) const;

  void load(const std::string &_assembly_path);
  void unload();

  template <typename T, typename... Rests>
  void collect_params(std::vector<void *> &_params, T &&arg, Rests &&...args) {
    _params.push_back(static_cast<void *>(&arg));
    collect_params(_params, std::forward<Rests>(args)...);
  }

  void collect_params(std::vector<void *> &_params) {}

  template <typename... Args>
  bool invoke(const script_ncm &ncm, Args... args) const {
    std::vector<void *> params;
    collect_params(params, std::forward<Args>(args)...);
    return invoke_impl(ncm, params);
  }

  template <typename Ret, typename... Args>
  bool invoke_r(const script_ncm &ncm, Ret &_result, Args... args) const {
    std::vector<void *> params;
    if constexpr (sizeof...(Args) > 0) {
      collect_params(params, std::forward<Args>(args)...);
    }
    return invoke_impl(ncm, params, invoke_result_v<Ret>, &_result);
  }

private:
  bool invoke_impl(const script_ncm &ncm,
                   const std::vector<void *> &params) const;

  bool invoke_impl(const script_ncm &ncm, const std::vector<void *> &params,
                   invoke_result result_type, void *result) const;

private:
  std::unique_ptr<invoker_impl> m_impl;
};

} // namespace mono_invoker