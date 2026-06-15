#include "instance.h"

#include "tests/cad/database/axis.h"
#include "tests/cad/database/datum.h"
#include "tests/cad/history/box_command.h"
#include "tests/cad/history/coordinate.h"
#include "tests/cad/document/command_panel.h"
#include "tests/cad/document/debug_panel.h"
#include "tests/cad/document/inspector_panel.h"
#include "tests/cad/interaction/command_dispatcher.h"
#include "tests/cad/interaction/doc_input_handler.h"
#include "tests/cad/interaction/ray_pick.h"
#include "tests/cad/interaction/selection_manager.h"
#include "tests/cad/renderer/highlight_renderer.h"
#include "tests/cad/renderer/viewport_axis.h"
#include "tests/cad/renderer/viewport_datum.h"

#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <array>
#include <GLFW/glfw3.h>

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

// Global shader with lighting (ambient + directional)
const char *k_global_vs = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uMVP;
uniform mat4 uModel;

out vec3 vWorldPos;
out vec3 vNormal;

void main() {
  vec4 worldPos = uModel * vec4(aPos, 1.0);
  vWorldPos = worldPos.xyz;
  vNormal = mat3(transpose(inverse(uModel))) * aNormal;
  gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

const char *k_global_fs = R"(#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3 uCameraPos;
uniform vec3 uLightDir;      // Directional light direction (normalized)
uniform vec3 uAmbientColor;  // Ambient light color
uniform vec3 uDiffuseColor;  // Diffuse light color
uniform vec3 uBaseColor;     // Object base color

out vec4 FragColor;

void main() {
  vec3 normal = normalize(vNormal);
  vec3 lightDir = normalize(-uLightDir); // Light direction from surface to light

  // Ambient
  vec3 ambient = uAmbientColor * uBaseColor;

  // Diffuse (Lambert)
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 diffuse = uDiffuseColor * diff * uBaseColor;

  // Simple view direction for specular hint (optional)
  vec3 viewDir = normalize(uCameraPos - vWorldPos);
  float spec = pow(max(dot(reflect(-lightDir, normal), viewDir), 0.0), 32.0);
  vec3 specular = vec3(0.1) * spec; // Weak specular

  vec3 result = ambient + diffuse + specular;
  FragColor = vec4(result, 1.0);
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
  m_global_shader.reset(shader::shader_from_source(k_global_vs, k_global_fs));
  viewport_axis::instance();
  viewport_datum::instance();

  // Initialize highlight renderer for selection visualization
  renderer::highlight_renderer::instance().init();

  // Register commands in command panel
  interaction::command_panel::instance().register_button(
      "Create Box", "Create a box by picking two diagonal corners",
      []() {
        interaction::command_dispatcher::instance().push_command(
            std::make_unique<box_command>());
      });

  // Register clear selection command
  interaction::command_panel::instance().register_button(
      "Clear Selection", "Clear all subshape selections",
      []() {
        interaction::selection_manager::instance().clear_selection();
        std::printf("[instance] Cleared all selections\n");
      });
}

void instance::update(float delta_time) {
  (void)delta_time;
  // Update active command state
  interaction::command_dispatcher::instance().update();
}

void instance::render() {
  // Solid BRep: cull back faces with CCW front (matches OCCT mesh winding)
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  m_history.draw_global();
  m_history.draw_local();

  // Draw command preview (e.g., box preview during creation)
  interaction::command_dispatcher::instance().draw();

  // Draw selection highlights (hover and selected subshapes)
  renderer::highlight_renderer::instance().render_highlights();

  m_viewport_axes.draw(m_disp.view_matrix);
}

void instance::render_ui() {
  m_history.draw_ui();
  interaction::inspector::instance().draw_ui();
  interaction::command_panel::instance().draw_ui();
  interaction::command_dispatcher::instance().draw_ui();
  interaction::debug_panel::instance().draw_ui();
}

bool instance::on_mouse_moved(double xpos, double ypos) {
  // Cache mouse position and ray pick coordinate for debug panel
  auto &dbg = interaction::debug_panel::instance();
  dbg.set_mouse_position(xpos, ypos);

  auto &picker = interaction::ray_pick::instance();
  glm::vec3 world_point;
  const bool valid = picker.pick_at(xpos, ypos, m_disp, world_point);
  dbg.set_ray_pick_coordinate(world_point, valid);

  // Forward to input handler stack (commands push handlers to receive input)
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
  // Forward to input handler stack (commands push handlers to receive input)
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
