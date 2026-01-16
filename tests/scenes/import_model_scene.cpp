#include "import_model_scene.h"
#include "shader_helper.h"

import_model_scene::import_model_scene()
    : renderable_scene_base("Import Model Scene"),
      m_import_model("assets/models/backpack/backpack.obj") {}

void import_model_scene::init(GLFWwindow *_window) {
  renderable_scene_base::init(_window);

  // Load shaders
  load_shader_pair("shaders/import_model_test/vertex.shader",
                   "shaders/import_model_test/fragment.shader",
                   "shaders/import_model_test/light_fragment.shader", m_shader,
                   m_light_shader);

  // Setup camera
  m_camera.m_position = {0.0f, 0.0f, 3.0f};
  m_camera.m_yaw = 0.0f;
  m_camera.m_pitch = 0.0f;
  m_camera.update_view_matrix();
}

void import_model_scene::render() {
  if (!m_shader) {
    return;
  }

  set_matrices(m_shader);
  m_import_model.draw(m_shader);
}

void import_model_scene::render_ui() { renderable_scene_base::render_ui(); }