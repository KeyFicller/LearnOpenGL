#include "selection_manager.h"

#include "tests/cad/history/box_feature.h"
#include "tests/cad/history/history_tree.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/ray_pick.h"

#include <cstdio>
#include <algorithm>
#include <cmath>
#include <limits>

namespace toy_cad::interaction {

namespace {

std::optional<std::pair<glm::vec2, glm::vec2>>
project_bounds_to_screen(const geometry::aabb &bounds,
                         const glm::mat4 &clip_from_world,
                         const glm::vec2 &viewport_size) {
  if (!bounds.valid() || viewport_size.x <= 0.0f || viewport_size.y <= 0.0f) {
    return std::nullopt;
  }

  const glm::vec3 corners[8] = {
      {bounds.min.x, bounds.min.y, bounds.min.z},
      {bounds.max.x, bounds.min.y, bounds.min.z},
      {bounds.max.x, bounds.max.y, bounds.min.z},
      {bounds.min.x, bounds.max.y, bounds.min.z},
      {bounds.min.x, bounds.min.y, bounds.max.z},
      {bounds.max.x, bounds.min.y, bounds.max.z},
      {bounds.max.x, bounds.max.y, bounds.max.z},
      {bounds.min.x, bounds.max.y, bounds.max.z},
  };

  glm::vec2 screen_min(std::numeric_limits<float>::max());
  glm::vec2 screen_max(std::numeric_limits<float>::lowest());
  bool any_in_front = false;

  for (const glm::vec3 &corner : corners) {
    const glm::vec4 clip = clip_from_world * glm::vec4(corner, 1.0f);
    if (clip.w <= 1e-6f) {
      continue;
    }

    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    const float screen_x =
        (ndc.x * 0.5f + 0.5f) * viewport_size.x;
    const float screen_y =
        (1.0f - (ndc.y * 0.5f + 0.5f)) * viewport_size.y;

    screen_min = glm::min(screen_min, glm::vec2(screen_x, screen_y));
    screen_max = glm::max(screen_max, glm::vec2(screen_x, screen_y));
    any_in_front = true;
  }

  if (!any_in_front) {
    return std::nullopt;
  }
  return std::make_pair(screen_min, screen_max);
}

bool screen_rect_fully_contains(const glm::vec2 &rect_min,
                                const glm::vec2 &rect_max,
                                const glm::vec2 &box_min,
                                const glm::vec2 &box_max) {
  return box_min.x >= rect_min.x && box_min.y >= rect_min.y &&
         box_max.x <= rect_max.x && box_max.y <= rect_max.y;
}

bool screen_rect_overlaps(const glm::vec2 &rect_min,
                          const glm::vec2 &rect_max,
                          const glm::vec2 &box_min,
                          const glm::vec2 &box_max) {
  return !(box_max.x < rect_min.x || box_min.x > rect_max.x ||
           box_max.y < rect_min.y || box_min.y > rect_max.y);
}

} // namespace

selection_manager &selection_manager::instance() {
  static selection_manager s;
  return s;
}

void selection_manager::clear() {
  clear_hover();
  clear_selection();
  cancel_drag_select();
}

void selection_manager::clear_hover() {
  m_hover_target.reset();
}

void selection_manager::clear_selection() {
  m_selection.clear();
  on_selection_changed();
}

bool selection_manager::update_hover(const glm::vec3 &ray_origin,
                                     const glm::vec3 &ray_dir) {
  const auto new_hover = ray_cast(ray_origin, ray_dir);

  const bool changed = (new_hover.has_value() != m_hover_target.has_value()) ||
                       (new_hover.has_value() && m_hover_target.has_value() &&
                        new_hover.value() != m_hover_target.value());

  m_hover_target = new_hover;
  return changed;
}

void selection_manager::set_hover_target(const subshape_ref &target) {
  m_hover_target = target;
}

void selection_manager::clear_hover_target() {
  m_hover_target.reset();
}

bool selection_manager::is_hovered(const subshape_ref &ref) const {
  return m_hover_target.has_value() && m_hover_target.value() == ref;
}

bool selection_manager::is_selected(const subshape_ref &ref) const {
  return m_selection.find(ref) != m_selection.end();
}

bool selection_manager::toggle_select(const subshape_ref &ref, bool multi) {
  if (!multi) {
    // Replace selection
    const bool already_selected = (m_selection.size() == 1) && is_selected(ref);
    m_selection.clear();

    if (!already_selected) {
      m_selection.insert(ref);
    }

    on_selection_changed();
    return true;
  } else {
    // Toggle in multi-select mode
    if (is_selected(ref)) {
      m_selection.erase(ref);
    } else {
      m_selection.insert(ref);
    }

    on_selection_changed();
    return true;
  }
}

void selection_manager::select(const subshape_ref &ref) {
  m_selection.insert(ref);
  on_selection_changed();
}

void selection_manager::deselect(const subshape_ref &ref) {
  m_selection.erase(ref);
  on_selection_changed();
}

void selection_manager::select_single(const subshape_ref &ref) {
  m_selection.clear();
  m_selection.insert(ref);
  on_selection_changed();
}

size_t selection_manager::box_select(const glm::vec2 &screen_min,
                                     const glm::vec2 &screen_max,
                                     const glm::vec2 &viewport_size,
                                     const glm::mat4 &clip_from_world,
                                     bool multi) {
  std::vector<subshape_ref> hits;
  screen_rect_select(screen_min, screen_max, viewport_size, clip_from_world,
                     box_select_mode::crossing, hits);

  if (!multi) {
    m_selection.clear();
  }

  for (const auto &hit : hits) {
    m_selection.insert(hit);
  }

  on_selection_changed();
  return hits.size();
}

void selection_manager::get_selection_vector(std::vector<subshape_ref> &out) const {
  out.clear();
  out.reserve(m_selection.size());
  for (const auto &ref : m_selection) {
    out.push_back(ref);
  }
}

void selection_manager::begin_drag_select(const glm::vec2 &start_pos) {
  m_drag_active = true;
  m_drag_start = start_pos;
  m_drag_current = start_pos;
  m_drag_preview.clear();
}

bool selection_manager::update_drag_select(const glm::vec2 &current_pos,
                                           const glm::vec2 &viewport_size,
                                           const glm::mat4 &clip_from_world) {
  if (!m_drag_active) {
    return false;
  }

  m_drag_current = current_pos;

  const glm::vec2 screen_min = glm::min(m_drag_start, m_drag_current);
  const glm::vec2 screen_max = glm::max(m_drag_start, m_drag_current);

  std::vector<subshape_ref> new_preview;
  screen_rect_select(screen_min, screen_max, viewport_size, clip_from_world,
                     drag_select_mode(), new_preview);

  const bool changed = (new_preview.size() != m_drag_preview.size()) ||
                       !std::equal(new_preview.begin(), new_preview.end(),
                                   m_drag_preview.begin());

  m_drag_preview = std::move(new_preview);
  return changed;
}

void selection_manager::end_drag_select(bool multi) {
  if (!m_drag_active) {
    return;
  }

  // Apply preview selection to actual selection
  if (!multi) {
    m_selection.clear();
  }

  for (const auto &ref : m_drag_preview) {
    m_selection.insert(ref);
  }

  m_drag_active = false;
  m_drag_preview.clear();
  on_selection_changed();
}

void selection_manager::cancel_drag_select() {
  m_drag_active = false;
  m_drag_preview.clear();
}

std::optional<std::pair<glm::vec2, glm::vec2>>
selection_manager::drag_rect() const {
  if (!m_drag_active) {
    return std::nullopt;
  }
  return std::make_pair(
      glm::min(m_drag_start, m_drag_current),
      glm::max(m_drag_start, m_drag_current));
}

std::optional<subshape_ref> selection_manager::ray_cast(
    const glm::vec3 &ray_origin, const glm::vec3 &ray_dir) const {
  // Iterate through all box_features in the history tree
  auto &db = instance::get().db();
  auto &hist = instance::get().history();

  float closest_t = std::numeric_limits<float>::max();
  std::optional<subshape_ref> closest_hit;

  for (const auto &h : hist) {
    auto *feature = db.try_get_as<box_feature>(h);
    if (!feature || !feature->has_geometry() || !feature->has_bvh()) {
      continue;
    }

    // Test ray against this feature's BVH
    const auto hit = feature->intersect_ray(ray_origin, ray_dir);
    if (hit.has_value() && hit->t < closest_t && hit->t > 0.0f) {
      closest_t = hit->t;
      closest_hit = subshape_ref{h, *hit->handle};
    }
  }

  return closest_hit;
}

void selection_manager::frustum_select(const glm::vec4 frustum_planes[6],
                                       std::vector<subshape_ref> &out_hits) const {
  out_hits.clear();

  auto &db = instance::get().db();
  auto &hist = instance::get().history();

  for (const auto &h : hist) {
    auto *feature = db.try_get_as<box_feature>(h);
    if (!feature || !feature->has_geometry() || !feature->has_bvh()) {
      continue;
    }

    std::vector<const geometry::subshape_handle *> hits;
    feature->intersect_frustum(frustum_planes, hits);

    for (const auto *subshape : hits) {
      if (subshape) {
        out_hits.push_back(subshape_ref{h, *subshape});
      }
    }
  }
}

void selection_manager::screen_rect_select(
    const glm::vec2 &screen_min, const glm::vec2 &screen_max,
    const glm::vec2 &viewport_size, const glm::mat4 &clip_from_world,
    box_select_mode mode, std::vector<subshape_ref> &out_hits) const {
  out_hits.clear();

  auto &db = instance::get().db();
  auto &hist = instance::get().history();

  for (const auto &h : hist) {
    auto *feature = db.try_get_as<box_feature>(h);
    if (!feature || !feature->has_geometry()) {
      continue;
    }

    auto test_subshape = [&](const geometry::subshape_handle &subshape) {
      const auto screen_bounds =
          project_bounds_to_screen(subshape.bounds(), clip_from_world,
                                   viewport_size);
      if (!screen_bounds.has_value()) {
        return;
      }

      const auto &[box_min, box_max] = screen_bounds.value();
      const bool match =
          (mode == box_select_mode::window)
              ? screen_rect_fully_contains(screen_min, screen_max, box_min,
                                           box_max)
              : screen_rect_overlaps(screen_min, screen_max, box_min, box_max);
      if (match) {
        out_hits.push_back(subshape_ref{h, subshape});
      }
    };

    for (const auto &face : feature->faces()) {
      test_subshape(face);
    }
    for (const auto &edge : feature->edges()) {
      test_subshape(edge);
    }
  }
}

void selection_manager::on_selection_changed() {
  // Debug output
  std::printf("[selection_manager] Selection changed: %zu items\n",
              m_selection.size());
}

} // namespace toy_cad::interaction
