#include "spline_movement_sub_scenes.h"
#include "glad/gl.h"
#include "glm/trigonometric.hpp"
#include "spline_movement_scene.h"

namespace spline_driving {

class spline {
public:
  spline(const glm::vec3 &_start_point, const glm::vec3 &_end_point,
         int _point_count) {
    m_points.resize(_point_count);
    m_orientations.resize(_point_count);
    m_segment_length =
        glm::length(_end_point - _start_point) / float(_point_count - 1);
    m_points[0] = _start_point;
    m_points[_point_count - 1] = _end_point;
    for (int i = 1; i < _point_count - 1; i++) {
      m_points[i] = _start_point + (float(i) / float(_point_count - 1)) *
                                       (_end_point - _start_point);
    }
    update_orientation();
  };
  ~spline() = default;

public:
  void resize_point(int _new_point_count) {
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
        glm::vec3 interpolated = glm::mix(smooth_curve[segment_idx],
                                          smooth_curve[segment_idx + 1], t);
        m_points.push_back(interpolated);
      }
    }

    m_segment_length =
        glm::length(m_points[m_points.size() - 1] - m_points[0]) /
        float(_new_point_count - 1);

    // Update orientations after resampling
    update_orientation();
  }

  void update_orientation() {
    if (m_points.size() != m_orientations.size()) {
      m_orientations.resize(m_points.size());
    }
    for (int i = 0; i < m_points.size() - 1; i++) {
      m_orientations[i] = glm::normalize(m_points[i + 1] - m_points[i]);
    }
    m_orientations[m_orientations.size() - 1] = glm::normalize(
        m_points[m_points.size() - 1] - m_points[m_points.size() - 2]);
  }

  void update_position(const glm::vec3 &_head) {
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
      m_points[i] = last_point +
                    final_direction * std::min(m_segment_length, segment_len);
      last_direction = final_direction;
    }
    update_orientation();
  }

  std::vector<glm::vec3> get_smooth_spline() const {
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

public:
  float m_segment_length = 0.1f;
  std::vector<glm::vec3> m_points;
  std::vector<glm::vec3> m_orientations;
  double m_minimum_rotation = glm::radians(45.0f);
  // -(t - 0.5)^2 + 0.2
  glm::vec3 m_shape_factor = glm::vec3(-3.0f, 3.0f, 0.6f);
};

} // namespace spline_driving

static spline_driving::spline gs_snake_spline(glm::vec3(0.0f, 0.0f, 0.0f),
                                              glm::vec3(1.0f, 0.0f, 0.0f), 10);

spline_movement_snake_sub_scene::spline_movement_snake_sub_scene(
    spline_movement_scene *_parent)
    : sub_scene<spline_movement_scene>(_parent, "Snake") {

  // Initialize points
  gs_snake_spline.resize_point(20);

  update_mesh_data();

  // Setup shader
  m_shaders[spline_shader_type::k_control_points] =
      new shader("shaders/spline_movement_test/control_point_vertex.shader",
                 "shaders/spline_movement_test/control_point_fragment.shader",
                 "shaders/spline_movement_test/control_point_geometry.shader");

  m_shaders[spline_shader_type::k_spline] =
      new shader("shaders/spline_movement_test/snake/body_vertex.shader",
                 "shaders/spline_movement_test/snake/body_fragment.shader",
                 "shaders/spline_movement_test/snake/body_geometry.shader");

  m_shaders[spline_shader_type::k_head] =
      new shader("shaders/spline_movement_test/snake/head_vertex.shader",
                 "shaders/spline_movement_test/snake/head_fragment.shader",
                 "shaders/spline_movement_test/snake/head_geometry.shader");
}

spline_movement_snake_sub_scene::~spline_movement_snake_sub_scene() {
  for (auto &shader : m_shaders) {
    delete shader.second;
  }
}

void spline_movement_snake_sub_scene::draw_snake() {
  {
    // Draw Control Points
    m_shaders[spline_shader_type::k_control_points]->use();
    m_shaders[spline_shader_type::k_control_points]->set_uniform(
        "uTotalPoints", static_cast<int>(gs_snake_spline.m_points.size()));
    m_shaders[spline_shader_type::k_control_points]->set_uniform(
        "uShapeFactor", gs_snake_spline.m_shape_factor);
    m_shaders[spline_shader_type::k_control_points]->set_uniform(
        "uBaseRadius", gs_snake_spline.m_segment_length * 0.3f);
    m_points_mesh_manager.bind();
    if (m_draw_control_points) {
      glDrawArrays(GL_POINTS, 0, m_points_mesh_manager.get_index_count());
    }
  }

  {
    // Draw Head
    m_shaders[spline_shader_type::k_head]->use();

    // Calculate head direction (from first point to second point)
    glm::vec3 headDirection =
        gs_snake_spline.m_points[0] - gs_snake_spline.m_points[1];
    if (glm::length(headDirection) < 0.0001f) {
      headDirection = glm::vec3(1.0f, 0.0f, 0.0f); // Default direction
    }

    m_shaders[spline_shader_type::k_head]->set_uniform("uHeadDirection",
                                                       headDirection);
    m_shaders[spline_shader_type::k_head]->set_uniform(
        "uHeadSize", gs_snake_spline.m_segment_length * 1.5f);
    m_shaders[spline_shader_type::k_head]->set_uniform(
        "uEyeRadius", gs_snake_spline.m_segment_length * 0.15f);
    m_shaders[spline_shader_type::k_head]->set_uniform(
        "uEyeOffset", gs_snake_spline.m_segment_length * 0.15f);

    m_points_mesh_manager.bind();
    glDrawArrays(GL_POINTS, 0, 1);
  }

  {
    // Draw Spline
    // Render smooth connecting lines with better appearance
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_shaders[spline_shader_type::k_spline]->use();
    m_shaders[spline_shader_type::k_spline]->set_uniform(
        "uLineWidth", gs_snake_spline.m_segment_length * 0.3f);
    m_shaders[spline_shader_type::k_spline]->set_uniform(
        "uShapeFactor", gs_snake_spline.m_shape_factor);
    m_shaders[spline_shader_type::k_spline]->set_uniform(
        "uTotalPoints", static_cast<int>(m_smooth_points.size()));
    m_line_strip_mesh_manager.bind();
    glDrawElements(GL_LINES, m_line_strip_mesh_manager.get_index_count(),
                   GL_UNSIGNED_INT, 0);
    glDisable(GL_BLEND);
  }
}

void spline_movement_snake_sub_scene::render() {
  // Configure stencil test for writing
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilMask(0xFF);

  // Draw normal object and write to stencil buffer
  for (auto &shader : m_shaders) {
    if (shader.first != spline_shader_type::k_head &&
        shader.first != spline_shader_type::k_spline) {
      continue;
    }
    shader.second->use();
    shader.second->set_uniform("uOffsetRatio", 1.0f);
  }
  draw_snake();

  // Draw boundary outline using stencil test
  glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
  glStencilMask(0x00); // Disable writing to stencil buffer
  glDisable(GL_DEPTH_TEST);

  for (auto &shader : m_shaders) {
    if (shader.first != spline_shader_type::k_head &&
        shader.first != spline_shader_type::k_spline) {
      continue;
    }
    shader.second->use();
    shader.second->set_uniform("uOffsetRatio", 1.1f);
  }
  draw_snake();

  glEnable(GL_DEPTH_TEST);
  glStencilMask(0xFF); // Re-enable stencil writing for next frame
}

void spline_movement_snake_sub_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Spline Movement - Snake");
  ImGui::Spacing();

  // TODO: handle update latter.
  int new_point_count = gs_snake_spline.m_points.size();
  if (ImGui::SliderInt("Total Points", &new_point_count, 10, 50)) {
  }
  if (new_point_count != gs_snake_spline.m_points.size()) {
    gs_snake_spline.resize_point(new_point_count);
    update_mesh_data();
  }
  // if (ImGui::SliderFloat("Segment Length", &m_segment_length, 0.01f, 0.2f)) {
  //   std::vector<glm::vec3> directions;
  //   for (int i = 1; i < m_points.size(); i++) {
  //     directions.push_back(glm::normalize(m_points[i] - m_points[i - 1]));
  //   }
  //   for (int i = 1; i < m_points.size(); i++) {
  //     m_points[i] = m_points[i - i] + directions[i] * m_segment_length;
  //   }
  //   update_mesh_data();
  // }

  ImGui::Checkbox("Draw Control Points", &m_draw_control_points);
}

void spline_movement_snake_sub_scene::update(float _delta_time) {
  gs_snake_spline.update_position(gs_snake_spline.m_points[0]);
}

bool spline_movement_snake_sub_scene::on_mouse_moved(double _xpos,
                                                     double _ypos) {
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  float viewport_width = static_cast<float>(viewport[2]);
  float viewport_height = static_cast<float>(viewport[3]);

  if (viewport_width <= 0 || viewport_height <= 0) {
    return false;
  }

  float x_screen = static_cast<float>(_xpos) - static_cast<float>(viewport[0]);
  float y_screen = static_cast<float>(_ypos) - static_cast<float>(viewport[1]);

  float x_norm = x_screen / viewport_width;
  float y_norm = y_screen / viewport_height;

  float x_clip = 2.0f * x_norm - 1.0f;
  float y_clip = 1.0f - 2.0f * y_norm;

  glm::vec3 clip_pos(x_clip, y_clip, 0.0f);

  if (!gs_snake_spline.m_points.empty()) {
    gs_snake_spline.update_position(clip_pos);
  }

  // Update mesh data
  update_mesh_data();

  return true;
}

void spline_movement_snake_sub_scene::update_mesh_data() {
  // Setup mesh
  mesh_data points_mesh_data(
      gs_snake_spline.m_points.data(),
      gs_snake_spline.m_points.size() * sizeof(glm::vec3),
      gs_snake_spline.m_points.size(), {vertex_attribute{3, GL_FLOAT, false}});
  m_points_mesh_manager.setup_mesh(points_mesh_data);

  // Setup line
  // Generate more points for smoother curve using Catmull-Rom spline
  m_smooth_points = gs_snake_spline.get_smooth_spline();
  std::vector<unsigned int> line_strip_indices;
  // Create indices for line
  for (size_t i = 0; i < m_smooth_points.size() - 1; i++) {
    line_strip_indices.push_back(static_cast<unsigned int>(i));
    line_strip_indices.push_back(static_cast<unsigned int>(i + 1));
  }

  mesh_data line_strip_mesh_data(
      m_smooth_points.data(), m_smooth_points.size() * sizeof(glm::vec3),
      line_strip_indices.data(), line_strip_indices.size(),
      {vertex_attribute{3, GL_FLOAT, false}});
  m_line_strip_mesh_manager.setup_mesh(line_strip_mesh_data);
}