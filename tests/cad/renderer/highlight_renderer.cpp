#include "highlight_renderer.h"

#include "tests/cad/history/box_feature.h"
#include "tests/cad/instance.h"
#include "tests/cad/interaction/selection_manager.h"
#include "tests/cad/renderer/disp_var.h"

#include "glad/gl.h"
#include "imgui.h"

#include <cstdio>
#include <cstring>
#include <cmath>

namespace toy_cad::renderer {

namespace {

void draw_dashed_rect(ImDrawList *draw_list, const ImVec2 &p0, const ImVec2 &p1,
                      ImU32 color, float thickness) {
  constexpr float k_dash = 6.0f;
  constexpr float k_gap = 4.0f;

  const auto dash_edge = [&](const ImVec2 &a, const ImVec2 &b) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1e-3f) {
      return;
    }
    const float ux = dx / len;
    const float uy = dy / len;
    for (float t = 0.0f; t < len; t += k_dash + k_gap) {
      const float t0 = t;
      const float t1 = std::min(t + k_dash, len);
      draw_list->AddLine(ImVec2(a.x + ux * t0, a.y + uy * t0),
                         ImVec2(a.x + ux * t1, a.y + uy * t1), color,
                         thickness);
    }
  };

  dash_edge(p0, ImVec2(p1.x, p0.y));
  dash_edge(ImVec2(p1.x, p0.y), p1);
  dash_edge(p1, ImVec2(p0.x, p1.y));
  dash_edge(ImVec2(p0.x, p1.y), p0);
}

} // namespace

highlight_renderer &highlight_renderer::instance() {
  static highlight_renderer s;
  return s;
}

highlight_renderer::~highlight_renderer() {
  cleanup();
}

void highlight_renderer::init() {
  if (m_initialized) {
    return;
  }

  init_shaders();
  init_buffers();

  m_initialized = true;
  std::printf("[highlight_renderer] Initialized\n");
}

void highlight_renderer::cleanup() {
  if (m_highlight_shader != 0) {
    glDeleteProgram(m_highlight_shader);
    m_highlight_shader = 0;
  }
  if (m_screen_quad_vao != 0) {
    glDeleteVertexArrays(1, &m_screen_quad_vao);
    glDeleteBuffers(1, &m_screen_quad_vbo);
    m_screen_quad_vao = 0;
    m_screen_quad_vbo = 0;
  }
  if (m_box_wire_vao != 0) {
    glDeleteVertexArrays(1, &m_box_wire_vao);
    glDeleteBuffers(1, &m_box_wire_vbo);
    m_box_wire_vao = 0;
    m_box_wire_vbo = 0;
  }
  m_initialized = false;
}

void highlight_renderer::init_shaders() {
  // Compile vertex shader
  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &k_highlight_vs, nullptr);
  glCompileShader(vs);

  // Compile fragment shader
  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &k_highlight_fs, nullptr);
  glCompileShader(fs);

  // Link program
  m_highlight_shader = glCreateProgram();
  glAttachShader(m_highlight_shader, vs);
  glAttachShader(m_highlight_shader, fs);
  glLinkProgram(m_highlight_shader);

  // Cleanup
  glDeleteShader(vs);
  glDeleteShader(fs);
}

void highlight_renderer::init_buffers() {
  // Screen quad for drag selection rectangle
  glGenVertexArrays(1, &m_screen_quad_vao);
  glGenBuffers(1, &m_screen_quad_vbo);

  // Bounding box wireframe
  glGenVertexArrays(1, &m_box_wire_vao);
  glGenBuffers(1, &m_box_wire_vbo);
}

void highlight_renderer::render_highlights() {
  if (!m_initialized) {
    init();
  }

  auto &sel_mgr = interaction::selection_manager::instance();

  // Get current display state
  auto &disp = instance::get().disp();
  const glm::mat4 mvp = disp.clip_from_world();

  // Store current GL state
  GLboolean depth_test_enabled;
  glGetBooleanv(GL_DEPTH_TEST, &depth_test_enabled);
  GLboolean blend_enabled;
  glGetBooleanv(GL_BLEND, &blend_enabled);
  GLint prev_polygon_mode;
  glGetIntegerv(GL_POLYGON_MODE, &prev_polygon_mode);

  // Setup for highlight rendering - use standard depth test
  // without polygon offset to maintain correct depth ordering
  if (!depth_test_enabled) {
    glEnable(GL_DEPTH_TEST);
  }
  glDepthFunc(GL_LESS);

  // Render hover highlight (yellow)
  if (auto hover = sel_mgr.hover_target()) {
    auto *feature = instance::get().db().try_get_as<box_feature>(hover->feature_handle);
    if (feature && feature->has_geometry()) {
      feature->draw_subshape_highlighted(hover->subshape,
                                         interaction::selection_colors::hover);
    }
  }

  // Render selected subshapes (green)
  std::vector<interaction::subshape_ref> selection;
  sel_mgr.get_selection_vector(selection);

  for (const auto &ref : selection) {
    auto *feature = instance::get().db().try_get_as<box_feature>(ref.feature_handle);
    if (feature && feature->has_geometry()) {
      // Check if this is also hovered (don't double-draw)
      if (!sel_mgr.hover_target().has_value() ||
          sel_mgr.hover_target().value() != ref) {
        feature->draw_subshape_highlighted(ref.subshape,
                                            interaction::selection_colors::selected);
      }
    }
  }

  // Render drag preview (light green)
  if (sel_mgr.is_drag_selecting()) {
    for (const auto &ref : sel_mgr.drag_preview()) {
      auto *feature =
          instance::get().db().try_get_as<box_feature>(ref.feature_handle);
      if (feature && feature->has_geometry()) {
        feature->draw_subshape_highlighted(ref.subshape,
                                           interaction::selection_colors::preview);
      }
    }
  }

  // Restore GL state
  if (!depth_test_enabled) {
    glDisable(GL_DEPTH_TEST);
  }
  glDepthFunc(GL_LESS);
  if (!blend_enabled) {
    glDisable(GL_BLEND);
  }
  glPolygonMode(GL_FRONT_AND_BACK, prev_polygon_mode);
}

void highlight_renderer::render_drag_rect_in_viewport() {
  auto &sel_mgr = interaction::selection_manager::instance();

  auto drag_rect_opt = sel_mgr.drag_rect();
  if (!drag_rect_opt.has_value()) {
    return;
  }

  const auto [min_rect, max_rect] = drag_rect_opt.value();
  const bool window_mode =
      sel_mgr.drag_select_mode() == interaction::box_select_mode::window;

  ImDrawList *draw_list = ImGui::GetWindowDrawList();
  if (!draw_list) {
    return;
  }

  const ImVec2 win_pos = ImGui::GetWindowPos();
  const ImVec2 content_min = ImGui::GetWindowContentRegionMin();
  const ImVec2 origin(win_pos.x + content_min.x, win_pos.y + content_min.y);

  const ImVec2 p0(origin.x + min_rect.x, origin.y + min_rect.y);
  const ImVec2 p1(origin.x + max_rect.x, origin.y + max_rect.y);

  if (window_mode) {
    draw_list->AddRectFilled(p0, p1, IM_COL32(80, 220, 80, 40));
    draw_list->AddRect(p0, p1, IM_COL32(80, 255, 80, 255), 0.0f, 0, 2.0f);
  } else {
    draw_list->AddRectFilled(p0, p1, IM_COL32(80, 140, 255, 40));
    draw_dashed_rect(draw_list, p0, p1, IM_COL32(80, 160, 255, 255), 2.0f);
  }
}

void highlight_renderer::render_subshape_highlight(const box_feature &feature,
                                                     const geometry::subshape_handle &subshape,
                                                     const glm::vec3 &color) {
  // This is handled by box_feature::draw_subshape_highlighted
  feature.draw_subshape_highlighted(subshape, color);
}

void highlight_renderer::render_subshapes_highlight(const box_feature &feature,
                                                     const std::vector<const geometry::subshape_handle *> &subshapes,
                                                     const glm::vec3 &color) {
  feature.draw_subshapes_highlighted(subshapes, color);
}

void highlight_renderer::draw_screen_rect(const glm::vec2 &min_rect,
                                         const glm::vec2 &max_rect,
                                         const glm::vec4 &color) {
  // Use ImGui for screen-space rectangle
  ImDrawList *draw_list = ImGui::GetBackgroundDrawList();
  if (!draw_list) {
    return;
  }

  // Fill rectangle
  draw_list->AddRectFilled(
      ImVec2(min_rect.x, min_rect.y),
      ImVec2(max_rect.x, max_rect.y),
      ImColor(color.x, color.y, color.z, color.w));
}

void highlight_renderer::draw_bounding_box(const geometry::aabb &box,
                                           const glm::vec3 &color) {
  if (!m_initialized) {
    return;
  }

  // Generate wireframe vertices for the bounding box
  const glm::vec3 vertices[8] = {
      {box.min.x, box.min.y, box.min.z},
      {box.max.x, box.min.y, box.min.z},
      {box.max.x, box.max.y, box.min.z},
      {box.min.x, box.max.y, box.min.z},
      {box.min.x, box.min.y, box.max.z},
      {box.max.x, box.min.y, box.max.z},
      {box.max.x, box.max.y, box.max.z},
      {box.min.x, box.max.y, box.max.z},
  };

  // Line indices for the 12 edges of a box
  const unsigned int indices[24] = {
      0, 1, 1, 2, 2, 3, 3, 0,  // Bottom face
      4, 5, 5, 6, 6, 7, 7, 4,  // Top face
      0, 4, 1, 5, 2, 6, 3, 7   // Vertical edges
  };

  // Upload vertices
  glBindVertexArray(m_box_wire_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_box_wire_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

  // Use highlight shader
  glUseProgram(m_highlight_shader);

  auto &disp = instance::get().disp();
  const GLint mvp_loc = glGetUniformLocation(m_highlight_shader, "uMVP");
  const GLint color_loc = glGetUniformLocation(m_highlight_shader, "uColor");

  glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &disp.clip_from_world()[0][0]);
  glUniform3f(color_loc, color.x, color.y, color.z);

  // Draw lines
  glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, indices);

  glBindVertexArray(0);
}

} // namespace toy_cad::renderer
