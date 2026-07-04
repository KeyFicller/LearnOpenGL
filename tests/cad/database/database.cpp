#include "database.h"

namespace toy_cad {

object *database::try_get(handle h) {
  if (!h.valid()) {
    return nullptr;
  }
  if (h.index >= m_slots.size()) {
    return nullptr;
  }
  slot &s = m_slots[h.index];
  if (!s.alive || s.generation != h.generation) {
    return nullptr;
  }
  return s.obj.get();
}

const object *database::try_get(handle h) const {
  if (!h.valid()) {
    return nullptr;
  }
  if (h.index >= m_slots.size()) {
    return nullptr;
  }
  const slot &s = m_slots[h.index];
  if (!s.alive || s.generation != h.generation) {
    return nullptr;
  }
  return s.obj.get();
}

bool database::destroy(handle h) {
  if (!h.valid()) {
    return false;
  }
  if (h.index >= m_slots.size()) {
    return false;
  }
  slot &s = m_slots[h.index];
  if (!s.alive || s.generation != h.generation) {
    return false;
  }
  s.obj.reset();
  s.alive = false;
  ++s.generation;
  m_free.push_back(h.index);
  return true;
}

void database::clear() {
  m_slots.clear();
  m_free.clear();
}

} // namespace toy_cad
