#include "soft_body_frog_scene.h"
#include "GLFW/glfw3.h"
#include "glad/gl.h"
#include "glm/gtc/type_ptr.hpp"
#include "tests/component/interaction_utils.h"
#include <cmath>
#include <cstddef>

soft_body_frog_scene::soft_body_frog_scene()
    : renderable_scene_base("Dumb Slime"),
      m_driver(glm::vec2(-1.0f, 1.0f), glm::vec2(-1.0f, 1.0f)) {}

soft_body_frog_scene::~soft_body_frog_scene() {
  delete m_body_shader;
  delete m_outline_shader;
  delete m_eye_shader;
  delete m_mouth_shader;
}

void soft_body_frog_scene::init(GLFWwindow *_window) {
  renderable_scene_base::init(_window);
  m_camera.Position = {0.0f, 0.0f, 3.0f};
  m_camera.Orthographic = true;
  m_camera.Left = -1.0f;
  m_camera.Right = 1.0f;
  m_camera.Bottom = -1.0f;
  m_camera.Top = 1.0f;
  m_camera.update_view_matrix();
  m_camera.update_projection_matrix();

  m_body_shader = new shader("shaders/soft_body_frog/body_vertex.shader",
                             "shaders/soft_body_frog/body_fragment.shader");
  m_outline_shader =
      new shader("shaders/soft_body_frog/outline_vertex.shader",
                 "shaders/soft_body_frog/outline_fragment.shader",
                 "shaders/soft_body_frog/outline_geometry.shader");
  m_eye_shader = new shader("shaders/soft_body_frog/eye_vertex.shader",
                            "shaders/soft_body_frog/eye_fragment.shader",
                            "shaders/soft_body_frog/eye_geometry.shader");
  m_mouth_shader = new shader("shaders/soft_body_frog/mouth_vertex.shader",
                              "shaders/soft_body_frog/mouth_fragment.shader",
                              "shaders/soft_body_frog/mouth_geometry.shader");

  init_frog_loop();
  update_mesh_data();
}

void soft_body_frog_scene::init_frog_loop() {
  const int n = 28;
  const float a = 0.14f;
  const float b = 0.18f;
  soft_body_point center;
  center.position = glm::vec2(0.0f, 0.0f);
  center.velocity = glm::vec2(0.0f, 0.0f);
  center.acceleration = glm::vec2(0.0f, -9.8f);

  const int base = static_cast<int>(m_driver.get_points().size());
  for (int i = 0; i < n; i++) {
    float t = i * 2.0f * static_cast<float>(M_PI) / n;
    soft_body_point p = center;
    p.position = glm::vec2(a * std::cos(t), b * std::sin(t));
    m_driver.add_point(p);
  }
  soft_body_loop loop;
  for (int i = 0; i < n; i++) {
    soft_body_segment seg;
    seg.index1 = base + i;
    seg.index2 = base + (i + 1) % n;
    seg.length = glm::length(m_driver.get_point(seg.index1).position -
                             m_driver.get_point(seg.index2).position);
    m_driver.add_segment(seg);
    loop.segment_indices.push_back(
        static_cast<int>(m_driver.get_segments().size()) - 1);
  }
  m_driver.add_loop(loop);
}

std::vector<int>
soft_body_frog_scene::get_loop_point_order(size_t _loop_index) const {
  const auto &loops = m_driver.get_loops();
  const auto &segments = m_driver.get_segments();
  if (_loop_index >= loops.size())
    return {};
  const auto &loop = loops[_loop_index];
  size_t np = m_driver.get_points().size();
  std::vector<std::vector<int>> adj(np);
  for (int si : loop.segment_indices) {
    if (si < 0 || static_cast<size_t>(si) >= segments.size())
      continue;
    const auto &s = segments[static_cast<size_t>(si)];
    size_t i1 = static_cast<size_t>(s.index1);
    size_t i2 = static_cast<size_t>(s.index2);
    if (i1 < np && i2 < np) {
      adj[i1].push_back(static_cast<int>(s.index2));
      adj[i2].push_back(static_cast<int>(s.index1));
    }
  }
  int start = -1;
  for (size_t i = 0; i < np; i++) {
    if (!adj[i].empty()) {
      start = static_cast<int>(i);
      break;
    }
  }
  if (start < 0)
    return {};
  std::vector<int> order;
  order.push_back(start);
  int prev = start;
  int cur = adj[static_cast<size_t>(start)][0];
  while (cur != start && order.size() <= np) {
    order.push_back(cur);
    int next = prev;
    for (int v : adj[static_cast<size_t>(cur)])
      if (v != prev) {
        next = v;
        break;
      }
    prev = cur;
    cur = next;
  }
  return order;
}

void soft_body_frog_scene::update_mesh_data() {
  const auto &points = m_driver.get_points();
  if (points.empty() || m_driver.get_loops().empty())
    return;

  std::vector<int> order = get_loop_point_order(0);
  if (order.size() < 3)
    return;

  glm::vec2 centroid(0.0f);
  for (int i : order)
    centroid += points[static_cast<size_t>(i)].position;
  centroid /= static_cast<float>(order.size());

  std::vector<glm::vec2> body_verts;
  body_verts.push_back(centroid);
  for (int i : order)
    body_verts.push_back(points[static_cast<size_t>(i)].position);
  body_verts.push_back(points[static_cast<size_t>(order[0])].position);

  mesh_data body_data(body_verts.data(), body_verts.size() * sizeof(glm::vec2),
                      body_verts.size(),
                      {vertex_attribute{2, GL_FLOAT, false}});
  m_body_mesh.setup_mesh(body_data);

  std::vector<glm::vec2> outline_verts;
  outline_verts.reserve(order.size());
  for (int i : order) {
    outline_verts.push_back(points[static_cast<size_t>(i)].position);
  }
  std::vector<unsigned int> outline_indices;
  for (size_t i = 0; i < outline_verts.size(); i++) {
    outline_indices.push_back(static_cast<unsigned int>(i));
    outline_indices.push_back(
        static_cast<unsigned int>((i + 1) % outline_verts.size()));
  }
  mesh_data outline_data(outline_verts.data(),
                         outline_verts.size() * sizeof(glm::vec2),
                         outline_indices.data(), outline_indices.size(),
                         {vertex_attribute{2, GL_FLOAT, false}});
  m_outline_mesh.setup_mesh(outline_data);

  glm::vec2 left_eye = centroid + glm::vec2(-m_eye_offset_x, m_eye_offset_y);
  glm::vec2 right_eye = centroid + glm::vec2(m_eye_offset_x, m_eye_offset_y);
  std::vector<glm::vec2> eye_centers = {left_eye, right_eye};
  mesh_data eye_data(eye_centers.data(), eye_centers.size() * sizeof(glm::vec2),
                     eye_centers.size(),
                     {vertex_attribute{2, GL_FLOAT, false}});
  m_eye_mesh.setup_mesh(eye_data);

  float mx = centroid.x;
  float my = centroid.y - 0.06f;
  std::vector<glm::vec2> mouth_verts = {
      glm::vec2(mx - m_mouth_scale, my + 0.01f),
      glm::vec2(mx - m_mouth_scale * 0.5f, my - 0.01f),
      glm::vec2(mx, my - 0.015f),
      glm::vec2(mx + m_mouth_scale * 0.5f, my - 0.01f),
      glm::vec2(mx + m_mouth_scale, my + 0.01f),
  };
  std::vector<unsigned int> mouth_indices;
  for (size_t i = 0; i + 1 < mouth_verts.size(); i++) {
    mouth_indices.push_back(static_cast<unsigned int>(i));
    mouth_indices.push_back(static_cast<unsigned int>(i + 1));
  }
  mesh_data mouth_data(mouth_verts.data(),
                       mouth_verts.size() * sizeof(glm::vec2),
                       mouth_indices.data(), mouth_indices.size(),
                       {vertex_attribute{2, GL_FLOAT, false}});
  m_mouth_mesh.setup_mesh(mouth_data);
}

void soft_body_frog_scene::render() {
  if (m_driver.get_loops().empty())
    return;

  glm::mat4 mvp = m_camera.ProjectionMatrix * m_camera.ViewMatrix;

  GLboolean depth_enabled = glIsEnabled(GL_DEPTH_TEST);
  if (depth_enabled)
    glDisable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  std::vector<int> order = get_loop_point_order(0);
  if (order.size() < 3)
    return;

  m_body_shader->use();
  m_body_shader->set_uniform("uMVP", mvp);
  m_body_shader->set_uniform("uBodyColor", m_body_color);
  m_body_mesh.bind();
  glDrawArrays(GL_TRIANGLE_FAN, 0,
               static_cast<GLsizei>(m_body_mesh.get_index_count()));

  m_outline_shader->use();
  m_outline_shader->set_uniform("uMVP", mvp);
  m_outline_shader->set_uniform("uOutlineColor", m_outline_color);
  m_outline_shader->set_uniform("uLineWidth", m_outline_width);
  m_outline_mesh.bind();
  glDrawElements(GL_LINES, m_outline_mesh.get_index_count(), GL_UNSIGNED_INT,
                 0);

  m_eye_mesh.bind();
  struct {
    float radius;
    glm::vec3 color;
  } eye_layers[] = {
      {m_eye_outer_radius, m_outline_color},
      {m_eye_teal_radius, m_body_color},
      {m_eye_inner_radius, m_eye_ring_color},
      {m_eye_pupil_radius, m_outline_color},
  };
  for (const auto &layer : eye_layers) {
    m_eye_shader->use();
    m_eye_shader->set_uniform("uMVP", mvp);
    m_eye_shader->set_uniform("uRadius", layer.radius);
    m_eye_shader->set_uniform("uColor", layer.color);
    glDrawArrays(GL_POINTS, 0, 2);
  }

  m_mouth_shader->use();
  m_mouth_shader->set_uniform("uMVP", mvp);
  m_mouth_shader->set_uniform("uColor", m_outline_color);
  m_mouth_shader->set_uniform("uLineWidth", m_outline_width * 0.8f);
  m_mouth_mesh.bind();
  glDrawElements(GL_LINES, m_mouth_mesh.get_index_count(), GL_UNSIGNED_INT, 0);

  glDisable(GL_BLEND);

  if (depth_enabled)
    glEnable(GL_DEPTH_TEST);
}

void soft_body_frog_scene::render_ui() {
  render_camera_ui();
  ImGui::Separator();
  ImGui::ColorEdit3("Body", glm::value_ptr(m_body_color));
  ImGui::ColorEdit3("Eye ring", glm::value_ptr(m_eye_ring_color));
  ImGui::SliderFloat("Outline width", &m_outline_width, 0.002f, 0.02f);
  ImGui::SliderFloat("Area strength", &m_driver.m_area_correction_strength,
                     0.0f, 1.0f);
}

void soft_body_frog_scene::update(float _delta_time) {
  renderable_scene_base::update(_delta_time);
  m_driver.update(_delta_time);

  if (m_dragging && m_drag_point_index >= 0 &&
      static_cast<size_t>(m_drag_point_index) < m_driver.get_points().size()) {
    auto &p = m_driver.get_point(m_drag_point_index);
    p.position = m_mouse_position;
    p.velocity = glm::vec2(0.0f);
  }

  update_mesh_data();
}

void soft_body_frog_scene::on_object_hovered(int _object_id) {
  if (_object_id == 1) {
    m_hovered_object = true;
  } else {
    m_hovered_object = false;
  }
}

bool soft_body_frog_scene::on_mouse_button(int _button, int _action,
                                           int _mods) {
  (void)_mods;
  if (_button == GLFW_MOUSE_BUTTON_LEFT && _action == GLFW_PRESS) {
    const auto &points = m_driver.get_points();
    if (points.empty())
      return false;

    float best_dist2 = 0.06f * 0.06f;
    int best_index = -1;
    for (size_t i = 0; i < points.size(); ++i) {
      glm::vec2 diff = points[i].position - m_mouse_position;
      float d2 = glm::dot(diff, diff);
      if (d2 <= best_dist2) {
        best_dist2 = d2;
        best_index = static_cast<int>(i);
      }
    }

    if (best_index >= 0) {
      m_dragging = true;
      m_drag_point_index = best_index;
      auto &p = m_driver.get_point(m_drag_point_index);
      p.position = m_mouse_position;
      p.velocity = glm::vec2(0.0f);
      return true;
    }
  } else if (_button == GLFW_MOUSE_BUTTON_LEFT && _action == GLFW_RELEASE) {
    m_dragging = false;
    m_drag_point_index = -1;
  }
  return false;
}

bool soft_body_frog_scene::on_mouse_moved(double _xpos, double _ypos) {
  if (!interaction_utils::screen_to_clip(_xpos, _ypos, m_mouse_position)) {
    return false;
  }

  if (m_dragging && m_drag_point_index >= 0 &&
      static_cast<size_t>(m_drag_point_index) < m_driver.get_points().size()) {
    auto &p = m_driver.get_point(m_drag_point_index);
    p.position = m_mouse_position;
    p.velocity = glm::vec2(0.0f);
    return true;
  }

  return false;
}