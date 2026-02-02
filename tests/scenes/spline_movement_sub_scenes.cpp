#include "spline_movement_sub_scenes.h"
#include "spline_movement_scene.h"

spline_movement_snake_sub_scene::spline_movement_snake_sub_scene(
    spline_movement_scene *_parent)
    : sub_scene<spline_movement_scene>(_parent, "Snake") {

  // Initialize points
  for (int i = 0; i < m_point_count; i++) {
    m_points.push_back(
        glm::vec3((i - m_point_count / 2) * m_segment_length, 0.0f, 0.0f));
  }

  update_mesh_data();

  // Setup shader
  m_shader = new shader("shaders/spline_movement_test/snake_vertex.shader",
                        "shaders/spline_movement_test/snake_fragment.shader",
                        "shaders/spline_movement_test/snake_geometry.shader");

  m_line_strip_shader =
      new shader("shaders/spline_movement_test/snake_vertex.shader",
                 "shaders/spline_movement_test/snake_fragment.shader");
}

spline_movement_snake_sub_scene::~spline_movement_snake_sub_scene() {
  delete m_shader;
}

void spline_movement_snake_sub_scene::render() {
  if (!m_shader) {
    return;
  }

  m_shader->use();

  m_points_mesh_manager.bind();
  glDrawArrays(GL_POINTS, 0, m_points_mesh_manager.get_index_count());

  m_line_strip_shader->use();
  m_line_strip_mesh_manager.bind();
  glDrawElements(GL_LINES, m_line_strip_mesh_manager.get_index_count(),
                 GL_UNSIGNED_INT, 0);
}

void spline_movement_snake_sub_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Spline Movement - Snake");
  ImGui::Spacing();

  // TODO: handle update latter.
  int new_point_count = m_points.size();
  if (ImGui::SliderInt("Point Count", &new_point_count, 10, 50)) {
  }
  if (new_point_count != m_points.size()) {
    int old_point_count = m_points.size();
    glm::vec3 last_dir =
        m_points[old_point_count - 1] - m_points[old_point_count - 2];
    m_points.resize(new_point_count);
    if (new_point_count > old_point_count) {
      for (int i = old_point_count; i < new_point_count; i++) {
        m_points[i] = m_points[i - 1] + last_dir;
      }
    }
    update_mesh_data();
  }
  if (ImGui::SliderFloat("Segment Length", &m_segment_length, 0.01f, 0.2f)) {
    std::vector<glm::vec3> directions;
    for (int i = 1; i < m_points.size(); i++) {
      directions.push_back(glm::normalize(m_points[i] - m_points[i - 1]));
    }
    for (int i = 1; i < m_points.size(); i++) {
      m_points[i] = m_points[i - i] + directions[i] * m_segment_length;
    }
    update_mesh_data();
  }
}

void spline_movement_snake_sub_scene::update(float _delta_time) {
  m_update_timer += _delta_time;
  if (m_update_timer >= m_update_frequency) {
    m_update_timer = 0.0f;
    // TODO: handle update.
    for (int i = 1; i < m_points.size(); i++) {
      auto last_point = m_points[i - 1];
      glm::vec3 direction = m_points[i] - last_point;
      if (glm::length(direction) > m_segment_length) {
        m_points[i] = last_point + glm::normalize(direction) * m_segment_length;
      }
    }
  }
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

  if (!m_points.empty()) {
    m_points[0] = clip_pos;
  }

  // Update mesh data
  update_mesh_data();

  return true;
}

void spline_movement_snake_sub_scene::update_mesh_data() {
  // Setup mesh
  mesh_data points_mesh_data(
      m_points.data(), m_points.size() * sizeof(glm::vec3), m_points.size(),
      {vertex_attribute{3, GL_FLOAT, false}});
  m_points_mesh_manager.setup_mesh(points_mesh_data);

  // Setup line strip mesh
  std::vector<unsigned int> line_strip_indices;
  for (int i = 0; i < m_points.size() - 1; i++) {
    line_strip_indices.push_back(i);
    line_strip_indices.push_back(i + 1);
  }
  mesh_data line_strip_mesh_data(
      m_points.data(), m_points.size() * sizeof(glm::vec3),
      line_strip_indices.data(), line_strip_indices.size(),
      {vertex_attribute{3, GL_FLOAT, false}});
  m_line_strip_mesh_manager.setup_mesh(line_strip_mesh_data);
}