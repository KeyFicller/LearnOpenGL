#include "spline_movement_sub_scenes.h"
#include "GLFW/glfw3.h"
#include "basic/shader.h"
#include "glad/gl.h"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/trigonometric.hpp"
#include "spline_movement_scene.h"
#include "tests/component/mesh_manager.h"
#include "tests/scenes/soft_body_dirver.h"

spline_movement_snake_sub_scene::spline_movement_snake_sub_scene(
    spline_movement_scene *_parent)
    : sub_scene<spline_movement_scene>(_parent, "Snake") {

  // Initialize points
  m_snake_spline.resize_point(20);

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

void spline_movement_snake_sub_scene::draw_spline() {
  {
    // Draw Control Points
    m_shaders[spline_shader_type::k_control_points]->use();
    m_shaders[spline_shader_type::k_control_points]->set_uniform(
        "uTotalPoints", static_cast<int>(m_snake_spline.m_points.size()));
    m_shaders[spline_shader_type::k_control_points]->set_uniform(
        "uShapeFactor", m_snake_spline.m_shape_factor);
    m_shaders[spline_shader_type::k_control_points]->set_uniform(
        "uBaseRadius", m_snake_spline.m_segment_length * 0.3f);
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
        m_snake_spline.m_points[0] - m_snake_spline.m_points[1];
    if (glm::length(headDirection) < 0.0001f) {
      headDirection = glm::vec3(1.0f, 0.0f, 0.0f); // Default direction
    }

    m_shaders[spline_shader_type::k_head]->set_uniform("uHeadDirection",
                                                       headDirection);
    m_shaders[spline_shader_type::k_head]->set_uniform(
        "uHeadSize", m_snake_spline.m_init_length * 0.075f);
    m_shaders[spline_shader_type::k_head]->set_uniform(
        "uEyeRadius", m_snake_spline.m_init_length * 0.0075f);
    m_shaders[spline_shader_type::k_head]->set_uniform(
        "uEyeOffset", m_snake_spline.m_init_length * 0.0075f);

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
        "uLineWidth", m_snake_spline.m_init_length * 0.015f);
    m_shaders[spline_shader_type::k_spline]->set_uniform(
        "uShapeFactor", m_snake_spline.m_shape_factor);
    m_shaders[spline_shader_type::k_spline]->set_uniform(
        "uTotalPoints", static_cast<int>(m_smooth_points.size()));
    m_line_strip_mesh_manager.bind();
    glDrawElements(GL_LINES, m_line_strip_mesh_manager.get_index_count(),
                   GL_UNSIGNED_INT, 0);
    glDisable(GL_BLEND);
  }
}

void spline_movement_snake_sub_scene::render() {
  glEnable(GL_STENCIL_TEST);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilMask(0xFF);

  for (auto &shader : m_shaders) {
    if (shader.first == spline_shader_type::k_control_points) {
      continue;
    }
    shader.second->use();
    shader.second->set_uniform("uOffsetRatio", 1.0f);
  }
  draw_spline();
  draw_attachments();

  glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
  glStencilMask(0x00);
  glDisable(GL_DEPTH_TEST);

  for (auto &shader : m_shaders) {
    if (shader.first == spline_shader_type::k_control_points) {
      continue;
    }
    shader.second->use();
    shader.second->set_uniform("uOffsetRatio", 1.15f);
  }
  draw_spline();
  draw_attachments();

  glEnable(GL_DEPTH_TEST);
  glStencilMask(0xFF);
  glDisable(GL_STENCIL_TEST);
}

void spline_movement_snake_sub_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Spline Movement - Snake");
  ImGui::Spacing();

  // TODO: handle update latter.
  int new_point_count = m_snake_spline.m_points.size();
  if (ImGui::SliderInt("Total Points", &new_point_count, 10, 50)) {
  }
  if (new_point_count != m_snake_spline.m_points.size()) {
    m_snake_spline.resize_point(new_point_count);
    update_mesh_data();
  }

  ImGui::Checkbox("Draw Control Points", &m_draw_control_points);
}

void spline_movement_snake_sub_scene::update(float _delta_time) {
  m_snake_spline.update_position(m_snake_spline.m_points[0]);
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

  if (!m_snake_spline.m_points.empty()) {
    m_snake_spline.update_position(clip_pos);
  }

  // Update mesh data
  update_mesh_data();

  return true;
}

void spline_movement_snake_sub_scene::update_mesh_data() {
  // Setup mesh
  mesh_data points_mesh_data(m_snake_spline.m_points.data(),
                             m_snake_spline.m_points.size() * sizeof(glm::vec3),
                             m_snake_spline.m_points.size(),
                             {vertex_attribute{3, GL_FLOAT, false}});
  m_points_mesh_manager.setup_mesh(points_mesh_data);

  // Setup line
  // Generate more points for smoother curve using Catmull-Rom spline
  m_smooth_points = m_snake_spline.get_smooth_spline();
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

spline_movement_lizard_sub_scene::spline_movement_lizard_sub_scene(
    spline_movement_scene *_parent)
    : spline_movement_snake_sub_scene(_parent) {

  m_name = "Lizard";

  // Set up legs
  std::tuple<float, float, bool> legs_param[4] = {{0.08f, 0.15f, true},
                                                  {0.08f, 0.15f, false},
                                                  {0.38f, 0.15f, true},
                                                  {0.38f, 0.15f, false}};
  for (int i = 0; i < 4; i++)
    m_snake_spline.add_attachment(new spline_driving::spline_leg_attachment(
        std::get<0>(legs_param[i]), std::get<1>(legs_param[i]),
        std::get<2>(legs_param[i])));

  // Set up shaders
  m_shaders[spline_shader_type::k_attachment] =
      new shader("shaders/spline_movement_test/lizard/leg_vertex.shader",
                 "shaders/spline_movement_test/lizard/leg_fragment.shader",
                 "shaders/spline_movement_test/lizard/leg_geometry.shader");
}

spline_movement_lizard_sub_scene::~spline_movement_lizard_sub_scene() {}

void spline_movement_lizard_sub_scene::render() {
  spline_movement_snake_sub_scene::render();
}

void spline_movement_lizard_sub_scene::draw_attachments() {
  for (int i = 0; i < 4; i++) {
    auto attached_leg = m_snake_spline.get_attachment(i);
    // Draw Control Points
    m_shaders[spline_shader_type::k_control_points]->use();
    m_shaders[spline_shader_type::k_control_points]->set_uniform(
        "uTotalPoints", static_cast<int>(attached_leg->get_points().size()));
    m_shaders[spline_shader_type::k_control_points]->set_uniform(
        "uShapeFactor", glm::vec3(0.0f, 0.0f, 0.6f));
    m_shaders[spline_shader_type::k_control_points]->set_uniform(
        "uBaseRadius", m_snake_spline.m_segment_length * 0.3f);
    m_legs_control_points_manager[i].bind();

    if (m_draw_control_points) {
      glDrawArrays(GL_POINTS, 0,
                   m_legs_control_points_manager[i].get_index_count());
    }

    m_shaders[spline_shader_type::k_attachment]->use();
    m_shaders[spline_shader_type::k_attachment]->set_uniform("uLegSize",
                                                             m_leg_size);
    m_legs_line_strip_manager[i].bind();
    glDrawElements(GL_LINES, m_legs_line_strip_manager[i].get_index_count(),
                   GL_UNSIGNED_INT, 0);
  }
}

void spline_movement_lizard_sub_scene::render_ui() {
  spline_movement_snake_sub_scene::render_ui();

  // // Debug
  // if (m_snake_spline.get_attachment(0)) {
  //   ImGui::Text("Elbow: %f, %f, %f",
  //               m_snake_spline.get_attachment(0)->get_points()[1].x,
  //               m_snake_spline.get_attachment(0)->get_points()[1].y,
  //               m_snake_spline.get_attachment(0)->get_points()[1].z);
  //   ImGui::Text("End: %f, %f, %f",
  //               m_snake_spline.get_attachment(0)->get_points()[2].x,
  //               m_snake_spline.get_attachment(0)->get_points()[2].y,
  //               m_snake_spline.get_attachment(0)->get_points()[2].z);
  // }

  ImGui::SliderFloat("Leg Size", &m_leg_size, 0.001f, 0.01f);
}

void spline_movement_lizard_sub_scene::update_mesh_data() {
  spline_movement_snake_sub_scene::update_mesh_data();

  for (int i = 0; i < 4; i++) {
    // Update legs mesh data
    auto attached_leg = m_snake_spline.get_attachment(i);
    mesh_data legs_mesh_data(attached_leg->get_points().data(),
                             attached_leg->get_points().size() *
                                 sizeof(glm::vec3),
                             attached_leg->get_points().size(),
                             {vertex_attribute{3, GL_FLOAT, false}});
    m_legs_control_points_manager[i].setup_mesh(legs_mesh_data);

    std::vector<unsigned int> line_strip_indices;
    // Create indices for line
    for (size_t i = 0; i < attached_leg->get_points().size() - 1; i++) {
      line_strip_indices.push_back(static_cast<unsigned int>(i));
      line_strip_indices.push_back(static_cast<unsigned int>(i + 1));
    }
    mesh_data legs_line_strip_mesh_data(
        attached_leg->get_points().data(),
        attached_leg->get_points().size() * sizeof(glm::vec3),
        line_strip_indices.data(), line_strip_indices.size(),
        {vertex_attribute{3, GL_FLOAT, false}});
    m_legs_line_strip_manager[i].setup_mesh(legs_line_strip_mesh_data);
  }
}

soft_body_sub_scene::soft_body_sub_scene(spline_movement_scene *_parent)
    : sub_scene<spline_movement_scene>(_parent, "Soft Body") {

  m_soft_body_dirver =
      soft_body_dirver(glm::vec2(-1.0f, 1.0f), glm::vec2(-1.0f, 1.0f));

  m_point_shader = new shader(
      "shaders/spline_movement_test/soft_body/flat_point_vertex.shader",
      "shaders/spline_movement_test/soft_body/flat_point_fragment.shader",
      "shaders/spline_movement_test/soft_body/flat_point_geometry.shader");
  m_segment_shader = new shader(
      "shaders/spline_movement_test/soft_body/flat_segment_vertex.shader",
      "shaders/spline_movement_test/soft_body/flat_segment_fragment.shader",
      "shaders/spline_movement_test/soft_body/flat_segment_geometry.shader");
}

soft_body_sub_scene::~soft_body_sub_scene() {
  delete m_point_shader;
  delete m_segment_shader;
}

void soft_body_sub_scene::render() {
  if (!m_soft_body_dirver.get_points().empty() ||
      !m_soft_body_dirver.get_segments().empty() ||
      !m_soft_body_dirver.get_loops().empty()) {
    m_point_shader->use();
    m_point_mesh.bind();
    glDrawArrays(GL_POINTS, 0, m_point_mesh.get_index_count());
  }

  if (!m_soft_body_dirver.get_segments().empty() ||
      !m_soft_body_dirver.get_loops().empty()) {
    m_segment_shader->use();
    m_segment_mesh.bind();
    glDrawElements(GL_LINES, m_segment_mesh.get_index_count(), GL_UNSIGNED_INT,
                   0);
  }
}

void soft_body_sub_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Soft Body");
  ImGui::Spacing();
  ImGui::Text("Mouse Position: %f, %f", m_mouse_position.x, m_mouse_position.y);
  static const char *hint_object_type[] = {"Point", "Segment", "Triangle",
                                           "Circle"};
  ImGui::Combo("Object Type", reinterpret_cast<int *>(&m_object_type),
               hint_object_type, IM_ARRAYSIZE(hint_object_type));
  ImGui::SliderFloat("Shape Factor",
                     &m_soft_body_dirver.m_area_correction_strength, 0.0f,
                     1.0f);
  if (ImGui::Button("Clear")) {
    m_soft_body_dirver.clear();
    // update_mesh_data();
  }
}

void soft_body_sub_scene::update(float _delta_time) {
  m_soft_body_dirver.update(_delta_time);

  update_mesh_data();
}

void soft_body_sub_scene::update_mesh_data() {
  std::vector<glm::vec2> coordinates;
  for (auto &point : m_soft_body_dirver.get_points()) {
    coordinates.push_back(point.position);
  }
  std::vector<unsigned int> segment_indices;
  for (auto &segment : m_soft_body_dirver.get_segments()) {
    segment_indices.push_back(segment.index1);
    segment_indices.push_back(segment.index2);
  }
  for (auto &loop : m_soft_body_dirver.get_loops()) {
    const auto &all_segments = m_soft_body_dirver.get_segments();
    for (int seg_idx : loop.segment_indices) {
      if (seg_idx >= 0 && static_cast<size_t>(seg_idx) < all_segments.size()) {
        const auto &seg = all_segments[static_cast<size_t>(seg_idx)];
        segment_indices.push_back(seg.index1);
        segment_indices.push_back(seg.index2);
      }
    }
  }
  mesh_data points_mesh_data(
      coordinates.data(), coordinates.size() * sizeof(glm::vec2),
      coordinates.size(), {vertex_attribute{2, GL_FLOAT, false}});
  m_point_mesh.setup_mesh(points_mesh_data);

  mesh_data segment_mesh_data(coordinates.data(),
                              coordinates.size() * sizeof(glm::vec2),
                              segment_indices.data(), segment_indices.size(),
                              {vertex_attribute{2, GL_FLOAT, false}});
  m_segment_mesh.setup_mesh(segment_mesh_data);
}

bool soft_body_sub_scene::on_mouse_button(int _button, int _action, int _mods) {
  if (_button == GLFW_MOUSE_BUTTON_LEFT && _action == GLFW_PRESS) {
    switch (m_object_type) {
    case object_type::k_point: {
      soft_body_point point;
      point.position = m_mouse_position;
      point.velocity = glm::vec2(m_random_distribution(m_random_engine),
                                 m_random_distribution(m_random_engine));
      point.acceleration = glm::vec2(0.0f, -9.8f);
      m_soft_body_dirver.add_point(point);
      break;
    }
    case object_type::k_segment: {
      glm::vec2 init_dir = glm::vec2(m_random_distribution(m_random_engine),
                                     m_random_distribution(m_random_engine));
      init_dir = glm::normalize(init_dir);
      // add two points
      soft_body_point point1;
      point1.position = m_mouse_position;
      point1.velocity = glm::vec2(m_random_distribution(m_random_engine),
                                  m_random_distribution(m_random_engine));
      point1.acceleration = glm::vec2(0.0f, -9.8f);
      m_soft_body_dirver.add_point(point1);
      soft_body_point point2;
      point2.position = m_mouse_position + init_dir * 0.2f;
      point2.velocity = glm::vec2(m_random_distribution(m_random_engine),
                                  m_random_distribution(m_random_engine));
      point2.acceleration = glm::vec2(0.0f, -9.8f);
      m_soft_body_dirver.add_point(point2);
      soft_body_segment segment;
      segment.index1 = m_soft_body_dirver.get_points().size() - 2;
      segment.index2 = m_soft_body_dirver.get_points().size() - 1;
      segment.length = 0.2f;
      m_soft_body_dirver.add_segment(segment);

      break;
    }
    case object_type::k_triangle:
    case object_type::k_circle: {
      soft_body_point center;
      center.position = m_mouse_position;
      center.velocity = glm::vec2(m_random_distribution(m_random_engine),
                                  m_random_distribution(m_random_engine));
      center.acceleration = glm::vec2(0.0f, -9.8f);

      const int point_count = m_object_type == object_type::k_triangle ? 3 : 20;
      const int base = static_cast<int>(m_soft_body_dirver.get_points().size());
      for (int i = 0; i < point_count; i++) {
        soft_body_point point = center;
        glm::vec2 direction = glm::vec2(
            glm::cos(i * 2.0f * static_cast<float>(M_PI) / point_count),
            glm::sin(i * 2.0f * static_cast<float>(M_PI) / point_count));
        point.position = center.position + direction * 0.2f;
        m_soft_body_dirver.add_point(point);
      }
      soft_body_loop loop;
      for (int i = 0; i < point_count; i++) {
        soft_body_segment segment;
        segment.index1 = base + i;
        segment.index2 = base + (i + 1) % point_count;
        segment.length =
            glm::length(m_soft_body_dirver.get_point(segment.index1).position -
                        m_soft_body_dirver.get_point(segment.index2).position);
        m_soft_body_dirver.add_segment(segment);
        loop.segment_indices.push_back(
            static_cast<int>(m_soft_body_dirver.get_segments().size()) - 1);
      }
      m_soft_body_dirver.add_loop(loop);
      break;
    }
    }

    update_mesh_data();

    return true;
  }
  return false;
}

bool soft_body_sub_scene::on_mouse_moved(double _xpos, double _ypos) {
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

  m_mouse_position = glm::vec2(x_clip, y_clip);

  return true;
}