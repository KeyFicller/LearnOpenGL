#include "instance_sub_scenes.h"
#include "instance_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_cube.h"
#include <glm/gtc/matrix_transform.hpp>

instance_uniform_pass_value_scene::instance_uniform_pass_value_scene(
    instance_scene *_parent)
    : sub_scene<instance_scene>(_parent, "Uniform Pass Value") {

  m_shader = new shader("shaders/instance_test/vertex_pass_by_uniform.shader",
                        "shaders/instance_test/fragment.shader");
};

instance_uniform_pass_value_scene::~instance_uniform_pass_value_scene() {
  delete m_shader;
}

void instance_uniform_pass_value_scene::render() {
  if (!m_shader) {
    return;
  }

  glm::vec2 translations[100];
  int index = 0;
  float offset = 0.1f;
  for (int y = -m_parent->m_col; y < m_parent->m_col; y += 2) {
    for (int x = -m_parent->m_row; x < m_parent->m_row; x += 2) {
      glm::vec2 translation;
      translation.x = x / (m_parent->m_row * 1.0f) * 5 + offset;
      translation.y = y / (m_parent->m_col * 1.0f) * 5 + offset;
      translations[index++] = translation;
    }
  }

  m_parent->set_matrices(m_shader,
                         glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)));
  m_shader->use();
  for (int i = 0; i < m_parent->m_row * m_parent->m_col; i++) {
    m_shader->set_uniform(("uTranslations[" + std::to_string(i) + "]").c_str(),
                          translations[i]);
  }
  m_parent->m_mesh.bind();
  glDrawElementsInstanced(GL_TRIANGLES, m_parent->m_mesh.get_index_count(),
                          GL_UNSIGNED_INT, 0,
                          m_parent->m_row * m_parent->m_col);
}

instance_attrib_pass_value_scene::instance_attrib_pass_value_scene(
    instance_scene *_parent)
    : sub_scene<instance_scene>(_parent, "Attrib Pass Value") {

  m_shader = new shader("shaders/instance_test/vertex_pass_by_attrib.shader",
                        "shaders/instance_test/fragment.shader");

  m_parent->m_mesh.bind();
  glGenBuffers(1, &m_instanceVBO);
  glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(m_translations), m_translations,
               GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glEnableVertexAttribArray(2);
  glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glVertexAttribDivisor(2, 1);
};

instance_attrib_pass_value_scene::~instance_attrib_pass_value_scene() {
  delete m_shader;
  glDeleteBuffers(1, &m_instanceVBO);
}

void instance_attrib_pass_value_scene::render() {
  if (!m_shader) {
    return;
  }

  int index = 0;
  float offset = 0.1f;
  for (int y = -m_parent->m_col; y < m_parent->m_col; y += 2) {
    for (int x = -m_parent->m_row; x < m_parent->m_row; x += 2) {
      glm::vec2 translation;
      translation.x = x / (m_parent->m_row * 1.0f) * 5 + offset;
      translation.y = y / (m_parent->m_col * 1.0f) * 5 + offset;
      m_translations[index++] = translation;
    }
  }
  glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(m_translations), m_translations,
               GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  m_parent->set_matrices(m_shader,
                         glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)));
  m_shader->use();
  m_parent->m_mesh.bind();
  glDrawElementsInstanced(GL_TRIANGLES, m_parent->m_mesh.get_index_count(),
                          GL_UNSIGNED_INT, 0,
                          m_parent->m_row * m_parent->m_col);
}