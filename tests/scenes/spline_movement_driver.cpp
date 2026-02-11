#include "spline_movement_driver.h"

namespace spline_driving {

// ========== spline class implementations ==========

spline::spline(const glm::vec3 &_start_point, const glm::vec3 &_end_point,
               int _point_count) {
  m_points.resize(_point_count);
  m_orientations.resize(_point_count);
  m_init_length = glm::length(_end_point - _start_point);
  m_segment_length = m_init_length / float(_point_count - 1);
  m_points[0] = _start_point;
  m_points[_point_count - 1] = _end_point;
  for (int i = 1; i < _point_count - 1; i++) {
    m_points[i] = _start_point + (float(i) / float(_point_count - 1)) *
                                     (_end_point - _start_point);
  }
  update_orientation();
}

spline::~spline() {
  for (auto &attachment : m_attachments) {
    delete attachment;
  }
}

void spline::resize_point(int _new_point_count) {
  if (_new_point_count < 2 || m_points.size() < 2) {
    return;
  }

  // Generate high-density smooth curve using Catmull-Rom spline
  std::vector<glm::vec3> smooth_curve;

  if (m_points.size() >= 2) {
    // Add first point
    smooth_curve.push_back(m_points[0]);

    // Generate smooth curve between points
    for (size_t i = 0; i < m_points.size() - 1; i++) {
      glm::vec3 p0 = (i > 0) ? m_points[i - 1] : m_points[i];
      glm::vec3 p1 = m_points[i];
      glm::vec3 p2 = m_points[i + 1];
      glm::vec3 p3 =
          (i + 2 < m_points.size()) ? m_points[i + 2] : m_points[i + 1];

      // Generate many intermediate points for accurate arc length calculation
      int segments = 32; // Higher density for better sampling
      for (int j = 1; j <= segments; j++) {
        float t = float(j) / float(segments);
        // Catmull-Rom spline
        float t2 = t * t;
        float t3 = t2 * t;
        glm::vec3 point =
            0.5f * (2.0f * p1 + (-p0 + p2) * t +
                    (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                    (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
        smooth_curve.push_back(point);
      }
    }
  }

  if (smooth_curve.size() < 2) {
    return;
  }

  // Calculate cumulative arc lengths
  std::vector<float> arc_lengths;
  arc_lengths.push_back(0.0f);
  float total_length = 0.0f;

  for (size_t i = 1; i < smooth_curve.size(); i++) {
    float segment_length = glm::length(smooth_curve[i] - smooth_curve[i - 1]);
    total_length += segment_length;
    arc_lengths.push_back(total_length);
  }

  if (total_length < 0.0001f) {
    return; // Curve is too short
  }

  // Resample points uniformly along the curve
  m_points.clear();
  m_points.reserve(_new_point_count);

  for (int i = 0; i < _new_point_count; i++) {
    float target_length =
        (total_length * float(i)) / float(_new_point_count - 1);

    // Find the segment containing target_length
    size_t segment_idx = 0;
    for (size_t j = 0; j < arc_lengths.size() - 1; j++) {
      if (target_length >= arc_lengths[j] &&
          target_length <= arc_lengths[j + 1]) {
        segment_idx = j;
        break;
      }
      if (j == arc_lengths.size() - 2) {
        segment_idx = j; // Last segment
      }
    }

    // Interpolate within the segment
    float segment_start = arc_lengths[segment_idx];
    float segment_end = arc_lengths[segment_idx + 1];
    float segment_length = segment_end - segment_start;

    if (segment_length < 0.0001f) {
      m_points.push_back(smooth_curve[segment_idx]);
    } else {
      float t = (target_length - segment_start) / segment_length;
      glm::vec3 interpolated =
          glm::mix(smooth_curve[segment_idx], smooth_curve[segment_idx + 1], t);
      m_points.push_back(interpolated);
    }
  }

  m_segment_length = m_init_length / float(_new_point_count - 1);

  // Update orientations after resampling
  update_orientation();

  // Update attachments after resizing points
  update_attachments();
}

void spline::update_orientation() {
  if (m_points.size() != m_orientations.size()) {
    m_orientations.resize(m_points.size());
  }
  for (int i = 0; i < m_points.size() - 1; i++) {
    m_orientations[i] = glm::normalize(m_points[i + 1] - m_points[i]);
  }
  m_orientations[m_orientations.size() - 1] = glm::normalize(
      m_points[m_points.size() - 1] - m_points[m_points.size() - 2]);
}

void spline::update_position(const glm::vec3 &_head) {
  if (glm::length(_head - m_points[0]) < 0.0001f) {
    return;
  }
  m_points[0] = _head;
  // Use head to derive initial direction hint
  glm::vec3 last_direction = glm::normalize(m_points[1] - _head);
  for (int i = 1; i < static_cast<int>(m_points.size()); i++) {
    glm::vec3 last_point = m_points[i - 1];

    glm::vec3 segment_vec = m_points[i] - last_point;
    float segment_len = glm::length(segment_vec);
    if (segment_len < 0.0001f) {
      // Degenerate segment, just continue with previous direction
      m_points[i] = last_point + last_direction * m_segment_length;
      continue;
    }

    glm::vec3 current_direction = segment_vec / segment_len;

    // Clamp rotation in XY plane between last_direction and current_direction
    glm::vec2 last2(last_direction.x, last_direction.y);
    glm::vec2 curr2(current_direction.x, current_direction.y);
    if (glm::length(last2) < 0.0001f) {
      last2 = glm::vec2(1.0f, 0.0f);
    } else {
      last2 = glm::normalize(last2);
    }
    if (glm::length(curr2) < 0.0001f) {
      curr2 = last2;
    } else {
      curr2 = glm::normalize(curr2);
    }

    float dot = glm::clamp(glm::dot(last2, curr2), -1.0f, 1.0f);
    float cross_z = last2.x * curr2.y - last2.y * curr2.x;
    float angle = std::atan2(cross_z, dot); // Signed angle in XY

    glm::vec3 final_direction;
    double max_turn = m_minimum_rotation;
    if (std::abs(angle) > max_turn) {
      // Rotate last_direction by clamped angle to stay within [
      // -m_minimum_rotation, m_minimum_rotation ]
      float target_angle =
          static_cast<float>((angle > 0.0f ? max_turn : -max_turn));
      float c = std::cos(target_angle);
      float s = std::sin(target_angle);
      glm::vec2 fixed2(c * last2.x - s * last2.y, s * last2.x + c * last2.y);
      fixed2 = glm::normalize(fixed2);
      final_direction = glm::normalize(glm::vec3(fixed2.x, fixed2.y, 0.0f));
    } else {
      final_direction = current_direction;
    }

    // Enforce segment length
    m_points[i] =
        last_point + final_direction * std::min(m_segment_length, segment_len);
    last_direction = final_direction;
  }
  update_orientation();

  update_attachments();
}

std::vector<glm::vec3> spline::get_smooth_spline() const {
  std::vector<glm::vec3> smooth_points;
  if (m_points.size() >= 2) {
    // Add first point
    smooth_points.push_back(m_points[0]);

    // Generate smooth curve between points
    for (size_t i = 0; i < m_points.size() - 1; i++) {
      glm::vec3 p0 = (i > 0) ? m_points[i - 1] : m_points[i];
      glm::vec3 p1 = m_points[i];
      glm::vec3 p2 = m_points[i + 1];
      glm::vec3 p3 =
          (i + 2 < m_points.size()) ? m_points[i + 2] : m_points[i + 1];

      // Generate 8 intermediate points for smooth curve
      int segments = 8;
      for (int j = 1; j <= segments; j++) {
        float t = float(j) / float(segments);
        // Catmull-Rom spline
        float t2 = t * t;
        float t3 = t2 * t;
        glm::vec3 point =
            0.5f * (2.0f * p1 + (-p0 + p2) * t +
                    (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                    (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
        smooth_points.push_back(point);
      }
    }
  }
  return smooth_points;
}

void spline::add_attachment(spline_attachment *_attachment) {
  m_attachments.push_back(_attachment);
}

void spline::update_attachments() {
  for (auto &attachment : m_attachments) {
    attachment->on_update(*this);
  }
}

// ========== spline_leg_attachment class implementations ==========

void spline_leg_attachment::on_attach(const spline &_spline) {
  m_segment_length = _spline.m_init_length * m_segment_ratio;
  m_segment_count = 2;

  int anchor_index = static_cast<int>(_spline.m_points.size() * m_attach_ratio);
  glm::vec3 anchor_point = _spline.m_points[anchor_index];
  glm::vec3 anchor_direction = _spline.m_orientations[anchor_index];

  glm::vec3 direction[2];
  direction[0] =
      glm::rotate(glm::mat4(1.0f), glm::radians(angle_multipler() * 120.f),
                  glm::vec3(0.0f, 0.0f, 1.0f)) *
      glm::vec4(anchor_direction, 1.0f);
  direction[1] =
      glm::rotate(glm::mat4(1.0f), glm::radians(angle_multipler() * -60.f),
                  glm::vec3(0.0f, 0.0f, 1.0f)) *
      glm::vec4(anchor_direction, 1.0f);

  m_points[0] = anchor_point;
  m_points[1] =
      m_points[0] + direction[0] * m_segment_length * m_lenth_ratio[0];
  m_points[2] =
      m_points[1] + direction[1] * m_segment_length * m_lenth_ratio[1];
}

void spline_leg_attachment::on_update(const spline &_spline) {
  int anchor_index = static_cast<int>(_spline.m_points.size() * m_attach_ratio);
  glm::vec3 anchor_point = _spline.m_points[anchor_index];

  // update anchor point
  m_points[0] = anchor_point;

  glm::vec3 anchor_direction = _spline.m_orientations[anchor_index];

  glm::vec3 prep_direction =
      glm::rotate(glm::mat4(1.0f), glm::radians(angle_multipler() * 90.f),
                  glm::vec3(0.0f, 0.0f, 1.0f)) *
      glm::vec4(anchor_direction, 1.0f);
  glm::vec3 target_point =
      anchor_point + prep_direction * m_segment_length * 0.7f;
  // update end point if too far
  if (glm::length(m_points.back() - target_point) > m_segment_length * 0.5f) {
    m_points.back() = target_point;
  }

  // update elbow point
  solve_3d_ik_xy_plane(m_points[0], m_points[2], m_segment_length * 0.6f,
                       m_segment_length * 0.4f, m_points[1], m_points[2]);
}

void spline_leg_attachment::solve_2d_ik(const glm::vec2 &_start,
                                        const glm::vec2 &_target,
                                        float _arm_length,
                                        float _forearm_length,
                                        glm::vec2 &_elbow, glm::vec2 &_end) {
  glm::vec2 to_target = _target - _start;
  float distance = glm::length(to_target);

  float max_reach = _arm_length + _forearm_length;
  float min_reach = std::abs(_arm_length - _forearm_length);

  if (distance > max_reach) {
    glm::vec2 dir = glm::normalize(to_target);
    _elbow = _start + dir * _arm_length;
    _end = _start + dir * max_reach;
    return;
  }

  if (distance < min_reach && min_reach > 0.001f) {
    glm::vec2 dir = glm::normalize(to_target);
    _elbow = _start + dir * _arm_length;
    _end = _start + dir * min_reach;
    return;
  }

  float cos_upper = (_arm_length * _arm_length + distance * distance -
                     _forearm_length * _forearm_length) /
                    (2.0f * _arm_length * distance);
  cos_upper = std::max(-1.0f, std::min(1.0f, cos_upper));
  float angle_upper = std::acos(cos_upper);

  glm::vec2 dir_to_target = glm::normalize(to_target);

  glm::vec2 perp = glm::vec2(-dir_to_target.y, dir_to_target.x);
  if (!m_is_left)
    perp = -perp;
  if (glm::length(perp) < 0.001f) {
    perp = glm::vec2(1.0f, 0.0f);
  } else {
    perp = glm::normalize(perp);
  }

  float c = std::cos(angle_upper);
  float s = std::sin(angle_upper);
  glm::vec2 elbow_dir = dir_to_target * c + perp * s;

  _elbow = _start + elbow_dir * _arm_length;
  _end = _target;
}

void spline_leg_attachment::solve_3d_ik_xy_plane(
    const glm::vec3 &_start, const glm::vec3 &_target, float _arm_length,
    float _forearm_length, glm::vec3 &_elbow, glm::vec3 &_end) {
  glm::vec2 start_2d = glm::vec2(_start.x, _start.y);
  glm::vec2 target_2d = glm::vec2(_target.x, _target.y);
  glm::vec2 elbow_2d;
  glm::vec2 end_2d;
  solve_2d_ik(start_2d, target_2d, _arm_length, _forearm_length, elbow_2d,
              end_2d);
  _elbow = glm::vec3(elbow_2d.x, elbow_2d.y, _start.z);
  _end = glm::vec3(end_2d.x, end_2d.y, _start.z);
}

const std::vector<glm::vec3> &spline_attachment::get_points() const {
  return m_points;
}

spline_attachment *spline::get_attachment(int _index) const {
  return m_attachments[_index];
}

} // namespace spline_driving
