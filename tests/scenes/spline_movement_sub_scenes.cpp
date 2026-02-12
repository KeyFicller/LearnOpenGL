#include "spline_movement_sub_scenes.h"
#include "glad/gl.h"
#include "glm/trigonometric.hpp"
#include "spline_movement_scene.h"
#include "tests/component/mesh_manager.h"

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
  // Configure stencil test for writing
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilMask(0xFF);

  // Draw normal object and write to stencil buffer
  for (auto &shader : m_shaders) {
    if (shader.first == spline_shader_type::k_control_points) {
      continue;
    }
    shader.second->use();
    shader.second->set_uniform("uOffsetRatio", 1.0f);
  }
  draw_spline();
  draw_attachments();

  // Draw boundary outline using stencil test
  glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
  glStencilMask(0x00); // Disable writing to stencil buffer
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
  glStencilMask(0xFF); // Re-enable stencil writing for next frame
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