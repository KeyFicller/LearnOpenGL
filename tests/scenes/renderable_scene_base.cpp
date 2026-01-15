#include "renderable_scene_base.h"

#include "glad/gl.h"
#include "glm/gtc/matrix_transform.hpp"

renderable_scene_base::renderable_scene_base(const std::string &_name)
    : camera_scene_base(_name) {}

renderable_scene_base::~renderable_scene_base() {
  delete m_shader;
  delete m_light_shader;
}

void renderable_scene_base::setup_mesh(const mesh_data &data) {
  m_mesh.setup_mesh(data);
}

void renderable_scene_base::set_matrices(shader *_shader,
                                         const glm::mat4 &model) {
  if (!_shader) {
    return;
  }
  _shader->use();
  _shader->set_uniform("model", model);
  _shader->set_uniform("view", m_camera.m_view_matrix);
  _shader->set_uniform("projection", m_camera.m_projection_matrix);
}

void renderable_scene_base::render_mesh() {
  if (m_shader) {
    set_matrices(m_shader);
    m_mesh.draw();
  }
}

void renderable_scene_base::render_light_source(shader *_light_shader,
                                                 const glm::vec3 &position,
                                                 const glm::vec3 &scale) {
  if (!_light_shader) {
    return;
  }
  glm::mat4 light_model = glm::mat4(1.0f);
  light_model = glm::translate(light_model, position);
  light_model = glm::scale(light_model, scale);
  set_matrices(_light_shader, light_model);
  m_mesh.draw();
}

