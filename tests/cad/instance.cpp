#include "instance.h"

#include "tests/cad/database/axis.h"
#include "tests/cad/database/datum.h"
#include "tests/cad/history/coordinate.h"
#include "tests/cad/interaction/doc_input_handler.h"
#include "tests/cad/interaction/inspector.h"
#include "tests/cad/renderer/viewport_axis.h"
#include "tests/cad/renderer/viewport_datum.h"

#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <array>

namespace toy_cad {

namespace {

const char *k_vp_vs = R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main() {
  gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

const char *k_vp_fs = R"(#version 330 core
uniform vec4 uColor;
out vec4 FragColor;
void main() {
  FragColor = uColor;
}
)";

} // namespace

void instance::push_input_handler(interaction::input_handler *handler) {
  m_input_stack.push_back(handler);
}

void instance::pop_input_handler() {
  if (!m_input_stack.empty()) {
    m_input_stack.pop_back();
  }
}

interaction::input_handler *instance::top_input_handler() {
  return m_input_stack.empty() ? nullptr : m_input_stack.back();
}

void instance::init(GLFWwindow *window) {
  m_window = window;
  m_database.clear();
  m_history.clear();
  m_input_stack.clear();

  interaction::doc_input_handler::instance().clear_selection();
  interaction::inspector::instance().clear_target();
  push_input_handler(&interaction::doc_input_handler::instance());

  std::array<handle, 3> dh{};
  std::array<handle, 3> ah{};
  for (int i = 0; i < 3; ++i) {
    dh[i] = m_database.emplace<datum>();
    ah[i] = m_database.emplace<axis>();
  }
  const handle coord = m_database.emplace<coordinate>(dh, ah);
  m_history.push_back(coord);

  if (auto *d0 = m_database.try_get_as<datum>(dh[0])) {
    d0->set_ax2(gp_Ax2(gp_Pnt(0., 0., 0.), gp_Dir(0., 0., 1.), gp_Dir(1., 0., 0.)));
  }
  if (auto *d1 = m_database.try_get_as<datum>(dh[1])) {
    d1->set_ax2(gp_Ax2(gp_Pnt(0., 0., 0.), gp_Dir(0., 1., 0.), gp_Dir(1., 0., 0.)));
  }
  if (auto *d2 = m_database.try_get_as<datum>(dh[2])) {
    d2->set_ax2(gp_Ax2(gp_Pnt(0., 0., 0.), gp_Dir(1., 0., 0.), gp_Dir(0., 1., 0.)));
  }
  if (auto *a0 = m_database.try_get_as<axis>(ah[0])) {
    a0->set_ax1(gp_Ax1(gp_Pnt(0., 0., 0.), gp_Dir(1., 0., 0.)));
  }
  if (auto *a1 = m_database.try_get_as<axis>(ah[1])) {
    a1->set_ax1(gp_Ax1(gp_Pnt(0., 0., 0.), gp_Dir(0., 1., 0.)));
  }
  if (auto *a2 = m_database.try_get_as<axis>(ah[2])) {
    a2->set_ax1(gp_Ax1(gp_Pnt(0., 0., 0.), gp_Dir(0., 0., 1.)));
  }

  m_vp_shader.reset(shader::shader_from_source(k_vp_vs, k_vp_fs));
  viewport_axis::instance();
  viewport_datum::instance();
}

void instance::update(float delta_time) { (void)delta_time; }

void instance::render() {
  m_history.draw_global();
  m_history.draw_local();
  m_viewport_axes.draw(m_disp.view_matrix);
}

void instance::render_ui() {
  m_history.draw_ui();
  interaction::inspector::instance().draw_ui();
}

bool instance::on_mouse_moved(double xpos, double ypos) {
  if (auto *top = top_input_handler()) {
    return top->on_mouse_moved(xpos, ypos);
  }
  return false;
}

bool instance::on_mouse_scroll(double xoffset, double yoffset) {
  if (auto *top = top_input_handler()) {
    return top->on_mouse_scroll(xoffset, yoffset);
  }
  return false;
}

bool instance::on_mouse_button(int button, int action, int mods) {
  if (auto *top = top_input_handler()) {
    return top->on_mouse_button(button, action, mods);
  }
  return false;
}

void instance::on_framebuffer_resized(int width, int height) {
  if (auto *top = top_input_handler()) {
    top->on_framebuffer_resized(width, height);
  }
}

void instance::on_object_hovered(int object_id) { (void)object_id; }

} // namespace toy_cad
