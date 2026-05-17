#include "viewport_axis.h"

#include "basic/shader.h"
#include "basic/vertex_array.h"

#include "glad/gl.h"
#include <imgui.h>
#include "imgui_internal.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace {

void apply_line_width_preferred(float preferred) {
  GLfloat range[2] = {1.f, 1.f};
  glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, range);
  const float lo = range[0];
  const float hi = range[1] > lo ? range[1] : lo;
  glLineWidth(std::clamp(preferred, lo, hi));
}

/** Fills `out` with triangle list: `n_seg` side tris + `n_seg` base cap tris (9 floats
 *  per triangle). Writes float count to `out_floats`. */
void axis_cone_positions(const glm::vec3 &tip, const glm::vec3 &dir_n, float base_r,
                         float height, int n_seg, float *out, size_t *out_floats) {
  const glm::vec3 up =
      glm::abs(dir_n.y) < 0.95f ? glm::vec3(0.f, 1.f, 0.f) : glm::vec3(1.f, 0.f, 0.f);
  const glm::vec3 u = glm::normalize(glm::cross(dir_n, up));
  const glm::vec3 v = glm::normalize(glm::cross(dir_n, u));
  const glm::vec3 apex = tip + dir_n * height;
  constexpr float k_two_pi = 6.28318530718f;
  size_t o = 0;
  auto emit_tri = [&](const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) {
    out[o++] = a.x;
    out[o++] = a.y;
    out[o++] = a.z;
    out[o++] = b.x;
    out[o++] = b.y;
    out[o++] = b.z;
    out[o++] = c.x;
    out[o++] = c.y;
    out[o++] = c.z;
  };
  for (int i = 0; i < n_seg; ++i) {
    const float t0 = k_two_pi * static_cast<float>(i) / static_cast<float>(n_seg);
    const float t1 =
        k_two_pi * static_cast<float>(i + 1) / static_cast<float>(n_seg);
    const glm::vec3 b0 =
        tip + base_r * (std::cos(t0) * u + std::sin(t0) * v);
    const glm::vec3 b1 =
        tip + base_r * (std::cos(t1) * u + std::sin(t1) * v);
    emit_tri(apex, b0, b1);
    emit_tri(tip, b0, b1);
  }
  *out_floats = o;
}

} // namespace

viewport_axis &viewport_axis::instance() {
  static viewport_axis s;
  return s;
}

viewport_axis::viewport_axis() {
  m_line_vao = new vertex_array();
  m_line_vbo = new vertex_buffer();
  m_line_vao->bind();
  m_line_vbo->bind();
  const float z6[6] = {};
  m_line_vbo->set_data(z6, sizeof(z6), GL_DYNAMIC_DRAW);
  m_line_vao->add_attributes({{3, GL_FLOAT, false}});
  m_line_vao->unbind();

  m_tip_vao = new vertex_array();
  m_tip_vbo = new vertex_buffer();
  m_tip_vao->bind();
  m_tip_vbo->bind();
  constexpr int k_max_seg = 24;
  constexpr size_t k_max_floats = static_cast<size_t>(k_max_seg) * 18u;
  const std::vector<float> zero(k_max_floats, 0.f);
  m_tip_vbo->set_data(zero.data(), zero.size() * sizeof(float), GL_DYNAMIC_DRAW);
  m_tip_vao->add_attributes({{3, GL_FLOAT, false}});
  m_tip_vao->unbind();
}

viewport_axis::~viewport_axis() {
  delete m_line_vao;
  delete m_line_vbo;
  delete m_tip_vao;
  delete m_tip_vbo;
}

void viewport_axis::draw_lines(shader &shader_prog, const glm::mat4 &mvp,
                               const glm::vec3 &p0, const glm::vec3 &p1,
                               const glm::vec4 &color) {
  shader_prog.set_uniform("uMVP", mvp);
  shader_prog.set_uniform("uColor", color);
  const float v[6] = {p0.x, p0.y, p0.z, p1.x, p1.y, p1.z};
  m_line_vao->bind();
  m_line_vbo->bind();
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
  apply_line_width_preferred(2.5f);
  glDrawArrays(GL_LINES, 0, 2);
  m_line_vao->unbind();
}

void viewport_axis::draw_axis_tip(shader &shader_prog, const glm::mat4 &mvp,
                                  const glm::vec3 &tip, const glm::vec3 &dir_unit,
                                  float base_radius, float cone_height,
                                  const glm::vec4 &color) {
  if (base_radius <= 0.f || cone_height <= 0.f) {
    return;
  }
  glm::vec3 d = dir_unit;
  const float len = glm::length(d);
  if (len < 1e-8f) {
    return;
  }
  d /= len;

  constexpr int k_seg = 20;
  constexpr size_t k_buf_floats = static_cast<size_t>(k_seg) * 18u;
  float buf[k_buf_floats];
  size_t nfloats = 0;
  axis_cone_positions(tip, d, base_radius, cone_height, k_seg, buf, &nfloats);

  shader_prog.set_uniform("uMVP", mvp);
  shader_prog.set_uniform("uColor", color);
  m_tip_vao->bind();
  m_tip_vbo->bind();
  glBufferSubData(GL_ARRAY_BUFFER, 0, nfloats * sizeof(float), buf);
  glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(nfloats / 3u));
  m_tip_vao->unbind();
}

void viewport_axis::draw_label(const glm::mat4 &mvp, int fbo_w, int fbo_h,
                               int sub_x, int sub_y, int sub_w, int sub_h,
                               const glm::vec3 &tip, const char *text,
                               std::uint32_t color_abgr) {
  if (fbo_w <= 0 || fbo_h <= 0 || sub_w <= 0 || sub_h <= 0 || !text[0])
    return;

  ImGuiWindow *win = ImGui::FindWindowByName("Scene Viewport");
  if (!win || win->Hidden)
    return;

  const ImRect inner = win->InnerRect;
  const float inner_w = inner.GetWidth();
  const float inner_h = inner.GetHeight();
  if (inner_w <= 1.f || inner_h <= 1.f)
    return;

  const glm::vec4 clip = mvp * glm::vec4(tip, 1.f);
  if (std::abs(clip.w) < 1e-6f)
    return;
  const float ndc_x = clip.x / clip.w;
  const float ndc_y = clip.y / clip.w;

  const float px = static_cast<float>(sub_x) +
                   (ndc_x * 0.5f + 0.5f) * static_cast<float>(sub_w);
  const float py_gl_bottom =
      static_cast<float>(sub_y) +
      (ndc_y * 0.5f + 0.5f) * static_cast<float>(sub_h);
  const float py_from_top = static_cast<float>(fbo_h) - 1.f - py_gl_bottom;

  const float sx =
      inner.Min.x + px * (inner_w / static_cast<float>(fbo_w));
  const float sy =
      inner.Min.y + py_from_top * (inner_h / static_cast<float>(fbo_h));

  const ImVec2 ts = ImGui::CalcTextSize(text);
  const ImVec2 origin(sx - ts.x * 0.5f, sy - ts.y * 0.5f);
  const ImRect text_bb(origin, ImVec2(origin.x + ts.x, origin.y + ts.y));
  if (text_bb.Min.x < inner.Min.x || text_bb.Max.x > inner.Max.x ||
      text_bb.Min.y < inner.Min.y || text_bb.Max.y > inner.Max.y) {
    return;
  }

  ImGui::GetForegroundDrawList()->AddText(
      origin, static_cast<ImU32>(color_abgr), text);
}
