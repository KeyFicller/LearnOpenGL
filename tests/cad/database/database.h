#pragma once

#include "handle.h"
#include "object.h"
#include "tests/cad/renderer/drawable_object.h"

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "name_generator.h"

namespace toy_cad {

class database {
  struct slot {
    std::unique_ptr<object> obj{};
    uint32_t generation = 0;
    bool alive = false;
  };

  std::vector<slot> m_slots{};
  std::vector<uint32_t> m_free{};

public:
  database() = default;
  database(const database &) = delete;
  database &operator=(const database &) = delete;
  database(database &&) = delete;
  database &operator=(database &&) = delete;

  object *try_get(handle h);
  const object *try_get(handle h) const;

  template <typename T, typename... Args>
  handle emplace(Args &&... args) {
    static_assert(std::is_base_of_v<object, T>);
    auto p = std::make_unique<T>(std::forward<Args>(args)...);
    assign_auto_tag_if_needed<T>(*p);
    uint32_t idx = 0;
    if (!m_free.empty()) {
      idx = m_free.back();
      m_free.pop_back();
      slot &s = m_slots[idx];
      s.obj = std::move(p);
      s.alive = true;
      return handle{idx, s.generation};
    }
    idx = static_cast<uint32_t>(m_slots.size());
    slot s;
    s.obj = std::move(p);
    s.generation = 0;
    s.alive = true;
    m_slots.push_back(std::move(s));
    return handle{idx, 0};
  }

  bool destroy(handle h);

  void clear();

  template <typename T>
  T *try_get_as(handle h) {
    return dynamic_cast<T *>(try_get(h));
  }

  template <typename T>
  const T *try_get_as(handle h) const {
    return dynamic_cast<const T *>(try_get(h));
  }

private:
  template <typename T>
  static void assign_auto_tag_if_needed(object &obj) {
    if constexpr (std::is_base_of_v<drawable_object, T>) {
      obj.set_tag(name_generator<T>::next());
    }
  }

};

} // namespace toy_cad
