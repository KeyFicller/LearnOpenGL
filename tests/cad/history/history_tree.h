#pragma once

#include "tests/cad/database/handle.h"
#include <cstddef>
#include <list>

namespace toy_cad {

class history_tree {
public:
  using iterator = std::list<handle>::iterator;
  using const_iterator = std::list<handle>::const_iterator;

  void push_back(handle h) { m_handles.push_back(h); }

  void pop_back() { m_handles.pop_back(); }

  void push_front(handle h) { m_handles.push_front(h); }

  void pop_front() { m_handles.pop_front(); }

  void remove(handle h) { m_handles.remove(h); }

  void clear() { m_handles.clear(); }

  size_t size() const { return m_handles.size(); }

  bool empty() const { return m_handles.empty(); }

  iterator begin() { return m_handles.begin(); }

  iterator end() { return m_handles.end(); }

  const_iterator begin() const { return m_handles.begin(); }

  const_iterator end() const { return m_handles.end(); }

  const_iterator cbegin() const { return m_handles.cbegin(); }

  const_iterator cend() const { return m_handles.cend(); }

  void draw_global();
  void draw_local();
  void draw_ui();

private:
  std::list<handle> m_handles{};
};

} // namespace toy_cad
