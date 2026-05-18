#include "viewport_axes_gizmo.h"

#include "tests/cad/instance.h"
#include "tests/cad/renderer/cad_display_constants.h"
#include "tests/cad/renderer/viewport_axis.h"
#include "tests/cad/renderer/viewport_datum.h"

#include "basic/shader.h"

#include "glad/gl.h"

#include <imgui.h>

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace toy_cad {

namespace {

constexpr int k_mini_side = 110;
/** Database datum + axes share one mini viewport size so they stay co-centered. */
constexpr int k_database_overlay_mini_side = 142;
constexpr int k_margin = 12;
constexpr float k_plane_extent = 0.9f;

struct gizmo_plane_pack {
  glm::mat4 mvp;
  glm::mat4 m_xy;
  glm::mat4 m_xz;
  glm::mat4 m_yz;
};

gizmo_plane_pack make_gizmo_plane_pack(const glm::mat4 &view_matrix) {
  const glm::mat3 R = glm::mat3(view_matrix);
  const glm::mat4 ortho = glm::ortho(-1.2f, 1.2f, -1.2f, 1.2f, -2.0f, 2.0f);
  const glm::mat4 mvp = ortho * glm::mat4(R);
  const float s = k_plane_extent;
  const glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(s, s, 1.f));
  const glm::mat4 t_xy =
      glm::translate(glm::mat4(1.f), glm::vec3(s * 0.5f, s * 0.5f, 0.f));
  const glm::mat4 t_xz =
      glm::translate(glm::mat4(1.f), glm::vec3(s * 0.5f, 0.f, s * 0.5f));
  const glm::mat4 t_yz =
      glm::translate(glm::mat4(1.f), glm::vec3(0.f, s * 0.5f, s * 0.5f));
  const glm::mat4 rx =
      glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
  const glm::mat4 ry = glm::rotate(glm::mat4(1.f), glm::radians(-90.f),
                                   glm::vec3(0.f, 1.f, 0.f));
  return {mvp, t_xy * S, t_xz * rx * S, t_yz * ry * S};
}

struct anchor_mini_rect {
  /** Visible subset of mini square (always inside parent viewport). */
  int draw_vx = 0;
  int draw_vy = 0;
  int draw_w = 0;
  int draw_h = 0;
  /** Keeps gizmo local origin on projected anchor when draw rect is a clipped subset. */
  float ndc_bias_x = 0.f;
  float ndc_bias_y = 0.f;
  int parent_vp[4]{};
};

glm::mat4 mvp_with_anchor_bias(const glm::mat4 &mvp, const anchor_mini_rect &ar) {
  if (ar.ndc_bias_x == 0.f && ar.ndc_bias_y == 0.f) {
    return mvp;
  }
  return glm::translate(glm::mat4(1.f),
                        glm::vec3(ar.ndc_bias_x, ar.ndc_bias_y, 0.f)) *
         mvp;
}

void begin_anchor_mini_gl_state(const anchor_mini_rect &ar) {
  glViewport(ar.draw_vx, ar.draw_vy, ar.draw_w, ar.draw_h);
  glEnable(GL_SCISSOR_TEST);
  glScissor(ar.draw_vx, ar.draw_vy, ar.draw_w, ar.draw_h);
}

/** Returns false only if anchor is behind the camera or mini square does not overlap
 * the parent viewport (center may lie outside; partial overlap still draws). */
bool try_compute_anchor_mini_rect(const glm::vec3 &world_anchor,
                                  const glm::mat4 &clip_from_world,
                                  int mini_side, anchor_mini_rect &out) {
  GLint vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  out.parent_vp[0] = vp[0];
  out.parent_vp[1] = vp[1];
  out.parent_vp[2] = vp[2];
  out.parent_vp[3] = vp[3];
  const int pw = vp[2];
  const int ph = vp[3];
  if (pw <= 0 || ph <= 0) {
    return false;
  }

  const glm::vec4 clip =
      clip_from_world *
      glm::vec4(world_anchor.x, world_anchor.y, world_anchor.z, 1.f);
  // Perspective: points behind the eye have w <= 0; ortho does not use this test.
  if (!instance::get().disp().orthographic && clip.w <= 0.f) {
    return false;
  }

  const int side = mini_side;
  if (side <= 0) {
    return false;
  }

  const float ndc_x = clip.x / clip.w;
  const float ndc_y = clip.y / clip.w;
  const float win_x = static_cast<float>(vp[0]) +
                      (ndc_x * 0.5f + 0.5f) * static_cast<float>(pw);
  const float win_y = static_cast<float>(vp[1]) +
                      (ndc_y * 0.5f + 0.5f) * static_cast<float>(ph);

  const float half = static_cast<float>(side) * 0.5f;
  const float mvx_f = win_x - half;
  const float mvy_f = win_y - half;
  const float pr_f = static_cast<float>(vp[0] + pw);
  const float pb_f = static_cast<float>(vp[1] + ph);
  const float px_f = static_cast<float>(vp[0]);
  const float py_f = static_cast<float>(vp[1]);

  const float sx0_f = std::max(mvx_f, px_f);
  const float sy0_f = std::max(mvy_f, py_f);
  const float sx1_f = std::min(mvx_f + static_cast<float>(side), pr_f);
  const float sy1_f = std::min(mvy_f + static_cast<float>(side), pb_f);
  if (sx1_f <= sx0_f || sy1_f <= sy0_f) {
    return false;
  }

  const int sx0 = static_cast<int>(std::floor(sx0_f));
  const int sy0 = static_cast<int>(std::floor(sy0_f));
  const int sx1 = static_cast<int>(std::ceil(sx1_f));
  const int sy1 = static_cast<int>(std::ceil(sy1_f));
  if (sx1 <= sx0 || sy1 <= sy0) {
    return false;
  }

  out.draw_vx = sx0;
  out.draw_vy = sy0;
  out.draw_w = sx1 - sx0;
  out.draw_h = sy1 - sy0;

  const float vw = static_cast<float>(out.draw_w);
  const float vh = static_cast<float>(out.draw_h);
  out.ndc_bias_x = 2.f * (win_x - (static_cast<float>(sx0) + vw * 0.5f)) / vw;
  out.ndc_bias_y = 2.f * (win_y - (static_cast<float>(sy0) + vh * 0.5f)) / vh;
  return true;
}

} // namespace

void viewport_axes_gizmo::draw_datum_planes_screen_fixed(
    const glm::vec3 &world_anchor, const glm::mat4 &view_matrix,
    const glm::mat4 &clip_from_world) {
  instance &inst = instance::get();
  shader *const sh = &inst.viewport_shader();
  anchor_mini_rect ar{};
  if (!try_compute_anchor_mini_rect(world_anchor, clip_from_world,
                                    k_database_overlay_mini_side, ar)) {
    return;
  }

  GLboolean depth_was = GL_TRUE;
  glGetBooleanv(GL_DEPTH_TEST, &depth_was);
  GLboolean blend_was = GL_FALSE;
  glGetBooleanv(GL_BLEND, &blend_was);
  GLint blend_src = GL_ONE, blend_dst = GL_ZERO;
  glGetIntegerv(GL_BLEND_SRC_RGB, &blend_src);
  glGetIntegerv(GL_BLEND_DST_RGB, &blend_dst);

  GLint scissor_box[4];
  GLboolean scissor_was = GL_FALSE;
  glGetBooleanv(GL_SCISSOR_TEST, &scissor_was);
  glGetIntegerv(GL_SCISSOR_BOX, scissor_box);

  begin_anchor_mini_gl_state(ar);

  const gizmo_plane_pack gp = make_gizmo_plane_pack(view_matrix);
  const glm::mat4 mvp = mvp_with_anchor_bias(gp.mvp, ar);
  viewport_datum &vd = viewport_datum::instance();

  sh->use();
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  vd.draw(*sh, mvp * gp.m_xy, glm::vec4(0.35f, 0.45f, 1.f, 0.22f));
  vd.draw(*sh, mvp * gp.m_xz, glm::vec4(0.25f, 1.f, 0.28f, 0.22f));
  vd.draw(*sh, mvp * gp.m_yz, glm::vec4(1.f, 0.28f, 0.25f, 0.22f));

  glUseProgram(0);

  glViewport(ar.parent_vp[0], ar.parent_vp[1], ar.parent_vp[2],
             ar.parent_vp[3]);
  if (scissor_was) {
    glScissor(scissor_box[0], scissor_box[1], scissor_box[2], scissor_box[3]);
    glEnable(GL_SCISSOR_TEST);
  } else {
    glDisable(GL_SCISSOR_TEST);
  }

  if (depth_was) {
    glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }

  if (blend_was) {
    glEnable(GL_BLEND);
    glBlendFunc(static_cast<GLenum>(blend_src), static_cast<GLenum>(blend_dst));
  } else {
    glDisable(GL_BLEND);
  }
}

void viewport_axes_gizmo::draw_axes_screen_fixed(
    const glm::vec3 &world_anchor, const glm::mat4 &view_matrix,
    const glm::mat4 &clip_from_world) {
  instance &inst = instance::get();
  shader *const sh = &inst.viewport_shader();
  anchor_mini_rect ar{};
  if (!try_compute_anchor_mini_rect(world_anchor, clip_from_world,
                                    k_database_overlay_mini_side, ar)) {
    return;
  }

  const int fbo_w = ar.parent_vp[2];
  const int fbo_h = ar.parent_vp[3];

  GLboolean depth_was = GL_TRUE;
  glGetBooleanv(GL_DEPTH_TEST, &depth_was);
  GLboolean blend_was = GL_FALSE;
  glGetBooleanv(GL_BLEND, &blend_was);
  GLint blend_src = GL_ONE, blend_dst = GL_ZERO;
  glGetIntegerv(GL_BLEND_SRC_RGB, &blend_src);
  glGetIntegerv(GL_BLEND_DST_RGB, &blend_dst);

  GLint scissor_box[4];
  GLboolean scissor_was = GL_FALSE;
  glGetBooleanv(GL_SCISSOR_TEST, &scissor_was);
  glGetIntegerv(GL_SCISSOR_BOX, scissor_box);

  begin_anchor_mini_gl_state(ar);

  const gizmo_plane_pack gp = make_gizmo_plane_pack(view_matrix);
  const glm::mat4 mvp = mvp_with_anchor_bias(gp.mvp, ar);
  viewport_axis &va = viewport_axis::instance();

  sh->use();
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  const float L = viewport_axis::k_axis_len;
  const glm::vec4 cx(1.f, 0.25f, 0.25f, 1.f);
  const glm::vec4 cy(0.25f, 1.f, 0.25f, 1.f);
  const glm::vec4 cz(0.35f, 0.45f, 1.f, 1.f);
  va.draw_lines(*sh, mvp, glm::vec3(0.f), glm::vec3(L, 0.f, 0.f), cx);
  va.draw_axis_tip(*sh, mvp, glm::vec3(L, 0.f, 0.f), glm::vec3(1.f, 0.f, 0.f),
                   k_axis_tip_cone_base_radius, k_axis_tip_cone_height, cx);
  va.draw_lines(*sh, mvp, glm::vec3(0.f), glm::vec3(0.f, L, 0.f), cy);
  va.draw_axis_tip(*sh, mvp, glm::vec3(0.f, L, 0.f), glm::vec3(0.f, 1.f, 0.f),
                   k_axis_tip_cone_base_radius, k_axis_tip_cone_height, cy);
  va.draw_lines(*sh, mvp, glm::vec3(0.f), glm::vec3(0.f, 0.f, L), cz);
  va.draw_axis_tip(*sh, mvp, glm::vec3(0.f, 0.f, L), glm::vec3(0.f, 0.f, 1.f),
                   k_axis_tip_cone_base_radius, k_axis_tip_cone_height, cz);

  glUseProgram(0);

  glViewport(ar.parent_vp[0], ar.parent_vp[1], ar.parent_vp[2],
             ar.parent_vp[3]);
  if (scissor_was) {
    glScissor(scissor_box[0], scissor_box[1], scissor_box[2], scissor_box[3]);
    glEnable(GL_SCISSOR_TEST);
  } else {
    glDisable(GL_SCISSOR_TEST);
  }

  if (depth_was) {
    glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }

  if (blend_was) {
    glEnable(GL_BLEND);
    glBlendFunc(static_cast<GLenum>(blend_src), static_cast<GLenum>(blend_dst));
  } else {
    glDisable(GL_BLEND);
  }

  constexpr float k_label_past_cone = 0.02f;
  const float tip = L + k_axis_tip_cone_height + k_label_past_cone;
  va.draw_label(mvp, fbo_w, fbo_h, ar.draw_vx, ar.draw_vy, ar.draw_w, ar.draw_h,
                glm::vec3(tip, 0.f, 0.f), "X", IM_COL32(255, 90, 90, 255));
  va.draw_label(mvp, fbo_w, fbo_h, ar.draw_vx, ar.draw_vy, ar.draw_w, ar.draw_h,
                glm::vec3(0.f, tip, 0.f), "Y", IM_COL32(90, 220, 90, 255));
  va.draw_label(mvp, fbo_w, fbo_h, ar.draw_vx, ar.draw_vy, ar.draw_w, ar.draw_h,
                glm::vec3(0.f, 0.f, tip), "Z", IM_COL32(120, 160, 255, 255));
}

void viewport_axes_gizmo::draw(const glm::mat4 &view_matrix) {
  instance &inst = instance::get();
  shader *const sh = &inst.viewport_shader();
  viewport_datum &vd = viewport_datum::instance();
  viewport_axis &va = viewport_axis::instance();

  GLint vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  const int fbo_w = vp[2];
  const int fbo_h = vp[3];
  if (fbo_w <= 0 || fbo_h <= 0)
    return;

  const int mini_vx = k_margin;
  const int mini_vy = k_margin;
  const int mini_side = k_mini_side;
  if (mini_vx + mini_side > fbo_w || mini_vy + mini_side > fbo_h)
    return;

  GLboolean depth_was = GL_TRUE;
  glGetBooleanv(GL_DEPTH_TEST, &depth_was);
  GLboolean blend_was = GL_FALSE;
  glGetBooleanv(GL_BLEND, &blend_was);
  GLint blend_src = GL_ONE, blend_dst = GL_ZERO;
  glGetIntegerv(GL_BLEND_SRC_RGB, &blend_src);
  glGetIntegerv(GL_BLEND_DST_RGB, &blend_dst);

  GLint scissor_box[4];
  GLboolean scissor_was = GL_FALSE;
  glGetBooleanv(GL_SCISSOR_TEST, &scissor_was);
  glGetIntegerv(GL_SCISSOR_BOX, scissor_box);

  glViewport(mini_vx, mini_vy, mini_side, mini_side);
  glEnable(GL_SCISSOR_TEST);
  glScissor(mini_vx, mini_vy, mini_side, mini_side);

  const gizmo_plane_pack gp = make_gizmo_plane_pack(view_matrix);

  sh->use();

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  vd.draw(*sh, gp.mvp * gp.m_xy, glm::vec4(0.35f, 0.45f, 1.f, 0.22f));
  vd.draw(*sh, gp.mvp * gp.m_xz, glm::vec4(0.25f, 1.f, 0.28f, 0.22f));
  vd.draw(*sh, gp.mvp * gp.m_yz, glm::vec4(1.f, 0.28f, 0.25f, 0.22f));

  glDisable(GL_BLEND);

  const float L = viewport_axis::k_axis_len;
  const glm::vec4 cx(1.f, 0.25f, 0.25f, 1.f);
  const glm::vec4 cy(0.25f, 1.f, 0.25f, 1.f);
  const glm::vec4 cz(0.35f, 0.45f, 1.f, 1.f);
  va.draw_lines(*sh, gp.mvp, glm::vec3(0.f), glm::vec3(L, 0.f, 0.f), cx);
  va.draw_axis_tip(*sh, gp.mvp, glm::vec3(L, 0.f, 0.f),
                   glm::vec3(1.f, 0.f, 0.f), k_axis_tip_cone_base_radius,
                   k_axis_tip_cone_height, cx);
  va.draw_lines(*sh, gp.mvp, glm::vec3(0.f), glm::vec3(0.f, L, 0.f), cy);
  va.draw_axis_tip(*sh, gp.mvp, glm::vec3(0.f, L, 0.f),
                   glm::vec3(0.f, 1.f, 0.f), k_axis_tip_cone_base_radius,
                   k_axis_tip_cone_height, cy);
  va.draw_lines(*sh, gp.mvp, glm::vec3(0.f), glm::vec3(0.f, 0.f, L), cz);
  va.draw_axis_tip(*sh, gp.mvp, glm::vec3(0.f, 0.f, L),
                   glm::vec3(0.f, 0.f, 1.f), k_axis_tip_cone_base_radius,
                   k_axis_tip_cone_height, cz);

  glUseProgram(0);

  glViewport(vp[0], vp[1], vp[2], vp[3]);
  if (scissor_was) {
    glScissor(scissor_box[0], scissor_box[1], scissor_box[2], scissor_box[3]);
    glEnable(GL_SCISSOR_TEST);
  } else {
    glDisable(GL_SCISSOR_TEST);
  }

  if (depth_was)
    glEnable(GL_DEPTH_TEST);
  else
    glDisable(GL_DEPTH_TEST);

  if (blend_was) {
    glEnable(GL_BLEND);
    glBlendFunc(static_cast<GLenum>(blend_src), static_cast<GLenum>(blend_dst));
  } else {
    glDisable(GL_BLEND);
  }

  constexpr float k_label_past_cone = 0.02f;
  const float tip = L + k_axis_tip_cone_height + k_label_past_cone;
  va.draw_label(gp.mvp, fbo_w, fbo_h, mini_vx, mini_vy, mini_side, mini_side,
                glm::vec3(tip, 0.f, 0.f), "X", IM_COL32(255, 90, 90, 255));
  va.draw_label(gp.mvp, fbo_w, fbo_h, mini_vx, mini_vy, mini_side, mini_side,
                glm::vec3(0.f, tip, 0.f), "Y", IM_COL32(90, 220, 90, 255));
  va.draw_label(gp.mvp, fbo_w, fbo_h, mini_vx, mini_vy, mini_side, mini_side,
                glm::vec3(0.f, 0.f, tip), "Z", IM_COL32(120, 160, 255, 255));
}

} // namespace toy_cad
