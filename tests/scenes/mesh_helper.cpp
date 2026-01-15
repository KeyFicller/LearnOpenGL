#include "mesh_helper.h"

#include "glad/gl.h"

mesh_helper::mesh_helper() = default;

mesh_helper::~mesh_helper() {
  delete m_VAO;
  delete m_VBO;
  delete m_EBO;
}

void mesh_helper::setup_mesh(const mesh_data &data) {
  // Clean up existing resources
  delete m_VAO;
  delete m_VBO;
  delete m_EBO;

  // Create VAO
  m_VAO = new vertex_array_object();
  m_VAO->bind();

  // Create and bind VBO
  m_VBO = new vertex_buffer_object();
  m_VBO->bind();
  m_VBO->set_data(data.vertices, data.vertex_size, GL_STATIC_DRAW);

  // Create and bind EBO
  m_EBO = new index_buffer_object();
  m_EBO->bind();
  m_EBO->set_data(data.indices, data.index_count * sizeof(unsigned int));

  // Set vertex attributes
  m_VAO->add_attributes(data.attributes);

  m_index_count = data.index_count;
}

void mesh_helper::bind() const {
  if (m_VAO) {
    m_VAO->bind();
  }
  if (m_EBO) {
    m_EBO->bind();
  }
}

void mesh_helper::draw() const {
  if (m_VAO && m_EBO && m_index_count > 0) {
    bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_index_count),
                   GL_UNSIGNED_INT, 0);
  }
}

