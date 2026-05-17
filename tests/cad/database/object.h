#pragma once

#include <string>

namespace toy_cad {

class database;

class object {
public:
  object(const object &) = delete;
  object &operator=(const object &) = delete;
  object(object &&) noexcept = default;
  object &operator=(object &&) noexcept = default;

  virtual ~object() = default;

  [[nodiscard]] const std::string &tag() const noexcept { return m_tag; }

  /** ImGui property panel sections; overrides call superclass first, then emit own fields. */
  virtual void inspect() const;

protected:
  object() = default;

private:
  friend class database;
  /** Only database::emplace assigns auto names; callers may specialize later via database API if added. */
  void set_tag(std::string tag) { m_tag = std::move(tag); }

  std::string m_tag{};
};

} // namespace toy_cad
