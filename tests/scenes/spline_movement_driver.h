#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/trigonometric.hpp"
#include <cmath>
#include <vector>

namespace spline_driving {

class spline_attachment;

class spline {
public:
  spline(const glm::vec3 &_start_point, const glm::vec3 &_end_point,
         int _point_count);
  virtual ~spline();

public:
  void resize_point(int _new_point_count);

  void update_orientation();

  void update_position(const glm::vec3 &_head);

  std::vector<glm::vec3> get_smooth_spline() const;

  void add_attachment(spline_attachment *_attachment);

  void update_attachments();

  spline_attachment *get_attachment(int _index) const;

public:
  float m_init_length = 1.0f;
  float m_segment_length = 0.1f;
  std::vector<glm::vec3> m_points;
  std::vector<glm::vec3> m_orientations;
  double m_minimum_rotation = glm::radians(45.0f);
  // -(t - 0.5)^2 + 0.2
  glm::vec3 m_shape_factor = glm::vec3(-3.0f, 3.0f, 0.6f);
  std::vector<spline_attachment *> m_attachments;
};

class spline_attachment {
public:
  spline_attachment(float _attach_ratio) : m_attach_ratio(_attach_ratio) {}
  virtual ~spline_attachment() = default;

  virtual void on_attach(const spline &_spline) = 0;
  virtual void on_update(const spline &_spline) = 0;

  const std::vector<glm::vec3> &get_points() const;

protected:
  float m_attach_ratio = 1.0f;
  std::vector<glm::vec3> m_points;
};

class spline_leg_attachment : public spline_attachment {
public:
  spline_leg_attachment(float _attach_ratio, float _length_ratio, bool _is_left)
      : spline_attachment(_attach_ratio), m_segment_ratio(_length_ratio),
        m_is_left(_is_left) {
    m_points.resize(3);
  }

  void on_attach(const spline &_spline) override;
  void on_update(const spline &_spline) override;
  float angle_multipler() const { return m_is_left ? -1.0f : 1.0f; }

private:
  void solve_2d_ik(const glm::vec2 &_start, const glm::vec2 &_target,
                   float _arm_length, float _forearm_length, glm::vec2 &_elbow,
                   glm::vec2 &_end);
  void solve_3d_ik_xy_plane(const glm::vec3 &_start, const glm::vec3 &_target,
                            float _arm_length, float _forearm_length,
                            glm::vec3 &_elbow, glm::vec3 &_end);

protected:
  float m_segment_ratio = 0.2f;
  float m_segment_length = 0.1f;
  float m_lenth_ratio[2] = {0.6f, 0.4f};
  int m_segment_count = 2;
  bool m_is_left = false;
};

} // namespace spline_driving