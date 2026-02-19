#pragma once

#include <memory>
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

/**
 * @brief Namespace, class, and method identifier for a script call
 */
struct script_ncm {
  script_namespace ns;
  script_class cls;
  script_method md;

  script_ncm(const script_namespace &_ns, const script_class &_cls,
             const script_method &_md)
      : ns(_ns), cls(_cls), md(_md) {}
};

/**
 * @brief Result type of an invoked method
 */
enum class invoke_result { k_integer, k_float, k_string };

template <typename tT>
struct invoke_result_helper;

template <>
struct invoke_result_helper<int> {
  static constexpr invoke_result value = invoke_result::k_integer;
};

template <>
struct invoke_result_helper<float> {
  static constexpr invoke_result value = invoke_result::k_float;
};

template <>
struct invoke_result_helper<std::string> {
  static constexpr invoke_result value = invoke_result::k_string;
};

template <typename tT>
inline constexpr invoke_result invoke_result_v =
    invoke_result_helper<tT>::value;

class invoker_impl;

/**
 * @brief Mono script invoker: load assembly in a dedicated AppDomain and
 * invoke static methods.
 */
class invoker {
  friend class invoker_impl;

public:
  invoker();
  ~invoker();

public:
  /**
   * @brief Whether an assembly is loaded and ready to invoke
   */
  bool is_ready() const;

  /**
   * @brief Invoke a static method and write string result
   * @param _ncm Namespace, class, and method
   * @param _result Output string
   * @return true on success
   */
  bool invoke(const script_ncm &_ncm, std::string &_result) const;

  void load(const std::string &_assembly_path);
  void unload();

  template <typename tT, typename... tRests>
  void collect_params(std::vector<void *> &_params, tT &&_arg,
                      tRests &&..._args) {
    _params.push_back(static_cast<void *>(&_arg));
    collect_params(_params, std::forward<tRests>(_args)...);
  }

  void collect_params(std::vector<void *> &_params) {}

  template <typename... tArgs>
  bool invoke(const script_ncm &_ncm, tArgs... _args) const {
    std::vector<void *> params;
    collect_params(params, std::forward<tArgs>(_args)...);
    return invoke_impl(_ncm, params);
  }

  template <typename tRet, typename... tArgs>
  bool invoke_r(const script_ncm &_ncm, tRet &_result, tArgs... _args) const {
    std::vector<void *> params;
    if constexpr (sizeof...(tArgs) > 0) {
      collect_params(params, std::forward<tArgs>(_args)...);
    }
    return invoke_impl(_ncm, params, invoke_result_v<tRet>, &_result);
  }

private:
  bool invoke_impl(const script_ncm &_ncm,
                   const std::vector<void *> &_params) const;

  bool invoke_impl(const script_ncm &_ncm,
                   const std::vector<void *> &_params,
                   invoke_result _result_type, void *_result) const;

private:
  std::unique_ptr<invoker_impl> m_impl;
};

} // namespace mono_invoker
