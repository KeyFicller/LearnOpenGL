#include "texture_cube_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_cube.h"
#include "tests/component/prefab_quad.h"
#include "tests/component/shader_loader.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

namespace {
float skyboxVertices[] = {
    // positions
    -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
    -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

    1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

    -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};
}

texture_cube_scene::texture_cube_scene()
    : renderable_scene_base("Texture Cube Scene") {}

texture_cube_scene::~texture_cube_scene() {
  delete m_texture_cube;
  delete m_skybox_shader;
}

void texture_cube_scene::init(GLFWwindow *window) {
  renderable_scene_base::init(window);

  mesh_data skybox_mesh_data = {skyboxVertices,
                                sizeof(skyboxVertices),
                                sizeof(skyboxVertices) / sizeof(float) / 3,
                                {{3, GL_FLOAT, false}}};
  m_skybox_mesh_manager.setup_mesh(skybox_mesh_data);

  m_texture_cube = new texture_cube(
      {"assets/images/skybox/right.jpg", "assets/images/skybox/left.jpg",
       "assets/images/skybox/top.jpg", "assets/images/skybox/bottom.jpg",
       "assets/images/skybox/front.jpg", "assets/images/skybox/back.jpg"});

  m_skybox_shader =
      load_shader("shaders/texture_cube_test/skybox_vertex.shader",
                  "shaders/texture_cube_test/skybox_fragment.shader");

  // Load some other info to draw something besides skybox
  prefab_cube::cube_mesh_data cube_data(
      prefab_cube::vertex_format::position_normal);
  setup_mesh(*cube_data.mesh);

  // Load shaders using helper
  load_shader_pair("shaders/texture_cube_test/vertex.shader",
                   "shaders/texture_cube_test/fragment.shader",
                   "shaders/texture_cube_test/light_fragment.shader", m_shader,
                   m_light_shader);

  // Initialize camera
  m_camera.m_position = {0.0f, 0.0f, 3.0f};
  m_camera.m_yaw = -90.0f; // Default looking along -Z axis
  m_camera.m_pitch = 0.0f;
  m_camera.update_view_matrix();
}

void texture_cube_scene::render() {
  if (!m_skybox_shader || !m_shader) {
    return;
  }

  // Draw some other object
  m_shader->use();
  set_matrices(m_shader);
  m_texture_cube->bind();
  m_shader->set_uniform("uTestMode", m_test_mode);
  m_shader->set_uniform("uEyePosition", m_camera.m_position);
  m_shader->set_uniform("uCubeMap", 0);
  m_shader->set_uniform("uRefractRatio", m_refract_ratio);
  m_mesh.draw();

  glDepthMask(GL_FALSE);
  glDepthFunc(GL_LEQUAL);
  m_skybox_shader->use();
  m_skybox_shader->set_uniform("model", glm::mat4(1.0f));
  m_skybox_shader->set_uniform("view",
                               glm::mat4(glm::mat3(m_camera.m_view_matrix)));
  m_skybox_shader->set_uniform("projection", m_camera.m_projection_matrix);

  m_texture_cube->bind();
  m_skybox_shader->set_uniform("uCubeMap", 0);
  m_skybox_mesh_manager.draw();
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LESS);
}

void texture_cube_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Texture Cube Scene");
  ImGui::Spacing();

  render_camera_ui();
  ImGui::Separator();

  ImGui::Combo("Test Mode", &m_test_mode, "Refract\0Reflect\0");
  if (m_test_mode == 0) {
    ImGui::SliderFloat("Refract Ratio", &m_refract_ratio, 0.0f, 1.0f);
  }
}