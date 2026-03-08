#pragma once

#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include <cmath>
#include <vector>

struct soft_body_point {
  glm::vec2 position;
  glm::vec2 velocity;
  glm::vec2 acceleration;
};

struct soft_body_segment {
  int index1;
  int index2;
  float length;
};

struct soft_body_loop {
  std::vector<int> segment_indices;
  float area;
  float rest_area;

  float calc_area(const std::vector<soft_body_point> &_points,
                  const std::vector<int> &_segment_indices,
                  const std::vector<soft_body_segment> &_all_segments) const {
    const size_t n = _points.size();
    if (n < 3 || _segment_indices.empty())
      return 0.0f;
    std::vector<std::vector<int>> adj(n);
    for (int si : _segment_indices) {
      if (si < 0 || static_cast<size_t>(si) >= _all_segments.size())
        continue;
      const auto &s = _all_segments[static_cast<size_t>(si)];
      size_t i1 = static_cast<size_t>(s.index1);
      size_t i2 = static_cast<size_t>(s.index2);
      if (i1 < n && i2 < n) {
        adj[i1].push_back(static_cast<int>(s.index2));
        adj[i2].push_back(static_cast<int>(s.index1));
      }
    }
    int start = -1;
    for (size_t i = 0; i < n; ++i)
      if (!adj[i].empty()) {
        start = static_cast<int>(i);
        break;
      }
    if (start < 0)
      return 0.0f;
    std::vector<int> order;
    order.push_back(start);
    int prev = start;
    int cur = adj[static_cast<size_t>(start)][0];
    while (cur != start && order.size() <= n) {
      order.push_back(cur);
      const auto &neighbors = adj[static_cast<size_t>(cur)];
      int next = prev;
      for (int v : neighbors)
        if (v != prev) {
          next = v;
          break;
        }
      prev = cur;
      cur = next;
    }
    if (order.size() < 3)
      return 0.0f;
    float sum = 0.0f;
    for (size_t i = 0; i < order.size(); ++i) {
      size_t j = (i + 1) % order.size();
      const glm::vec2 &a = _points[static_cast<size_t>(order[i])].position;
      const glm::vec2 &b = _points[static_cast<size_t>(order[j])].position;
      sum += a.x * b.y - b.x * a.y;
    }
    return 0.5f * std::abs(sum);
  }
};

class soft_body_dirver {
public:
  soft_body_dirver(glm::vec2 _horizontal_bound, glm::vec2 _vertical_bound)
      : m_horizontal_bound(_horizontal_bound),
        m_vertical_bound(_vertical_bound) {}

public:
  void update_point(soft_body_point &_point, float _delta_time) {
    _point.velocity += _point.acceleration * _delta_time;
    _point.velocity *= (1.0f - m_drag * _delta_time);

    auto next_position = _point.position + _point.velocity * _delta_time;
    bool hit_wall = false;

    if (next_position.x < m_horizontal_bound.x + m_boundary_tolerance ||
        next_position.x > m_horizontal_bound.y - m_boundary_tolerance) {
      _point.velocity.x = -_point.velocity.x * m_bounce_damping;
      next_position.x = glm::clamp(next_position.x,
                                   m_horizontal_bound.x + m_boundary_tolerance,
                                   m_horizontal_bound.y - m_boundary_tolerance);
      hit_wall = true;
    }
    if (next_position.y < m_vertical_bound.x + m_boundary_tolerance ||
        next_position.y > m_vertical_bound.y - m_boundary_tolerance) {
      _point.velocity.y = -_point.velocity.y * m_bounce_damping;
      next_position.y =
          glm::clamp(next_position.y, m_vertical_bound.x + m_boundary_tolerance,
                     m_vertical_bound.y - m_boundary_tolerance);
      hit_wall = true;
    }

    if (hit_wall && glm::length(_point.velocity) < m_velocity_stop_threshold)
      _point.velocity = glm::vec2(0.0f);

    _point.position = next_position;
  }

  void project_segment_length(soft_body_segment &_segment) {
    glm::vec2 &p1 = get_point(_segment.index1).position;
    glm::vec2 &p2 = get_point(_segment.index2).position;
    glm::vec2 center = (p1 + p2) * 0.5f;
    glm::vec2 edge = p2 - p1;
    float len = glm::length(edge);
    if (len < 1e-6f)
      return;
    glm::vec2 direction = edge / len;
    p1 = center - direction * _segment.length * 0.5f;
    p2 = center + direction * _segment.length * 0.5f;
  }

  void update_segment(soft_body_segment &_segment, float _delta_time) {
    (void)_delta_time;
    project_segment_length(_segment);
  }

  void update_loop(soft_body_loop &_loop, float _delta_time) {
    (void)_delta_time;
    float new_area =
        _loop.calc_area(m_points, _loop.segment_indices, m_segments);
    float rest = std::max(_loop.rest_area, 1e-6f);
    float area_error = (_loop.rest_area - new_area) / rest;
    float factor = area_error * m_area_correction_strength;

    for (int seg_idx : _loop.segment_indices) {
      if (seg_idx < 0 || static_cast<size_t>(seg_idx) >= m_segments.size())
        continue;
      soft_body_segment &segment = m_segments[static_cast<size_t>(seg_idx)];
      glm::vec2 edge = get_point(segment.index2).position -
                       get_point(segment.index1).position;
      float len = glm::length(edge);
      if (len < 1e-6f)
        continue;
      glm::vec2 perpendicular = glm::vec2(-edge.y / len, edge.x / len);

      get_point(segment.index1).velocity += factor * perpendicular;
      get_point(segment.index2).velocity += factor * perpendicular;
    }
    _loop.area = new_area;
  }

  void update(float _delta_time) {
    m_last_points = m_points;
    for (auto &loop : m_loops) {
      update_loop(loop, _delta_time);
    }
    for (auto &point : m_points) {
      update_point(point, _delta_time);
    }
    for (int iter = 0; iter < m_segment_constraint_iterations; ++iter) {
      for (auto &segment : m_segments) {
        project_segment_length(segment);
      }
    }
    for (size_t i = 0; i < m_points.size(); ++i) {
      glm::vec2 delta = m_points[i].position - m_last_points[i].position;
      m_points[i].velocity = delta / _delta_time;
      if (glm::length(m_points[i].velocity) < m_velocity_stop_threshold)
        m_points[i].velocity = glm::vec2(0.0f);
    }
  }

  void add_point(const soft_body_point &_point) { m_points.push_back(_point); }
  soft_body_point &get_point(int _index) { return m_points[_index]; }
  void add_segment(const soft_body_segment &_segment) {
    m_segments.push_back(_segment);
  }
  void add_loop(const soft_body_loop &_loop) {
    m_loops.push_back(_loop);
    if (!m_loops.back().segment_indices.empty()) {
      float a = m_loops.back().calc_area(
          m_points, m_loops.back().segment_indices, m_segments);
      m_loops.back().area = a;
      m_loops.back().rest_area = a;
    }
  }
  const std::vector<soft_body_point> &get_points() const { return m_points; }
  const std::vector<soft_body_segment> &get_segments() const {
    return m_segments;
  }
  const std::vector<soft_body_loop> &get_loops() const { return m_loops; }

  float m_area_correction_strength = 0.4f;

  void clear() {
    m_points.clear();
    m_segments.clear();
    m_loops.clear();
    m_last_points.clear();
  }

protected:
  glm::vec2 m_horizontal_bound;
  glm::vec2 m_vertical_bound;
  std::vector<soft_body_point> m_points;
  std::vector<soft_body_segment> m_segments;
  std::vector<soft_body_loop> m_loops;
  std::vector<soft_body_point> m_last_points;

  float m_boundary_tolerance = 0.05f;
  float m_drag = 2.0f;
  float m_bounce_damping = 0.7f;
  float m_velocity_stop_threshold = 0.01f;
  int m_segment_constraint_iterations = 32;
};