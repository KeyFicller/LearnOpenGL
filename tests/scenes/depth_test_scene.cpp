#include "depth_test_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_cube.h"
#include "tests/component/shader_loader.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

static const char *depth_test_mode_names[] = {
    "ALWAYS", "NEVER",   "LESS",     "EQUAL",
    "LEQUAL", "GREATER", "NOTEQUAL", "GEQUAL",
};

static std::vector<int> depth_test_modes = {
    GL_ALWAYS, GL_NEVER,   GL_LESS,     GL_EQUAL,
    GL_LEQUAL, GL_GREATER, GL_NOTEQUAL, GL_GEQUAL,
};

depth_test_scene::depth_test_scene()
    : renderable_scene_base("Depth Test Scene") {}

void depth_test_scene::init(GLFWwindow *_window) {
  renderable_scene_base::init(_window);

  // Setup mesh using cube helper
  prefab_cube::cube_mesh_data cube_data(
      prefab_cube::vertex_format::position_normal);
  setup_mesh(cube_data.mesh);

  // Load shaders using helper
  load_shader_pair("shaders/depth_test_test/vertex.shader",
                   "shaders/depth_test_test/fragment.shader",
                   "shaders/depth_test_test/light_fragment.shader", m_shader,
                   m_light_shader);

  // Initialize camera - position at (10, 10, 10) looking at origin
  m_camera.m_position = {10.0f, 10.0f, 10.0f};
  m_camera.m_yaw = -135.0f; // Looking from (10,10,10) towards (0,0,0)
  m_camera.m_pitch = -35.26f;
  m_camera.update_view_matrix();
}

void depth_test_scene::render() {
  if (!m_shader) {
    return;
  }

  glDepthFunc(depth_test_modes[m_depth_test_index]);
  // Set matrices and render object
  m_shader->use();
  m_shader->set_uniform<float, 3>("uLightColor", &m_light_color.x);
  m_shader->set_uniform<float, 3>("uObjectColor", &m_object_color.x);
  m_shader->set_uniform<float, 1>("uAmbientStrength", &m_ambient_strength);
  m_shader->set_uniform<float, 3>("uLightPosition", &m_light_position.x);
  m_shader->set_uniform<float, 1>("uSpecularStrength", &m_specular_strength);
  m_shader->set_uniform<float, 3>("uEyePosition", &m_camera.m_position.x);
  m_shader->set_uniform<float, 1>("uShininess", &m_shininess);
  m_shader->set_uniform<bool, 1>("uDepthVisualization", &m_depth_visualization);
  m_shader->set_uniform<float, 1>("uNear", &m_camera.m_near);
  m_shader->set_uniform<float, 1>("uFar", &m_camera.m_far);
  {
    set_matrices(m_shader, glm::translate(glm::mat4(1.0f), m_object_position));
    m_mesh.draw();
  }

  {
    set_matrices(m_shader, glm::translate(glm::mat4(1.0f), m_object_position2));
    m_mesh.draw();
  }
  glDepthFunc(GL_LESS);

  // Render light source using helper
  render_light_source(m_light_shader, m_light_position);
}

void depth_test_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Depth Test Scene");
  ImGui::Spacing();

  render_camera_ui();
  ImGui::SliderFloat3("Object Position", &m_object_position.x, -1.0f, 1.0f);
  ImGui::SliderFloat3("Object Position 2", &m_object_position2.x, -1.0f, 1.0f);

  ImGui::Combo("Depth Test Mode", &m_depth_test_index, depth_test_mode_names,
               IM_ARRAYSIZE(depth_test_mode_names));

  ImGui::Checkbox("Depth Visualization", &m_depth_visualization);
}
