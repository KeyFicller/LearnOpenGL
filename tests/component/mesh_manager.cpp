#include "tests/component/mesh_manager.h"

#include "glad/gl.h"

mesh_manager::mesh_manager() = default;

mesh_manager::~mesh_manager() {
  delete m_VAO;
  delete m_VBO;
  delete m_EBO;
}

mesh_manager::mesh_manager(mesh_manager &&other) noexcept
    : m_VAO(other.m_VAO), m_VBO(other.m_VBO), m_EBO(other.m_EBO),
      m_index_count(other.m_index_count) {
  other.m_VAO = nullptr;
  other.m_VBO = nullptr;
  other.m_EBO = nullptr;
  other.m_index_count = 0;
}

mesh_manager &mesh_manager::operator=(mesh_manager &&other) noexcept {
  if (this != &other) {
    // Clean up existing resources
    delete m_VAO;
    delete m_VBO;
    delete m_EBO;

    // Move resources from other
    m_VAO = other.m_VAO;
    m_VBO = other.m_VBO;
    m_EBO = other.m_EBO;
    m_index_count = other.m_index_count;

    // Reset other
    other.m_VAO = nullptr;
    other.m_VBO = nullptr;
    other.m_EBO = nullptr;
    other.m_index_count = 0;
  }
  return *this;
}

void mesh_manager::setup_mesh(const mesh_data &data) {
  // Clean up existing resources
  delete m_VAO;
  delete m_VBO;
  delete m_EBO;

  // Create VAO
  m_VAO = new vertex_array();
  m_VAO->bind();

  // Create and bind VBO
  m_VBO = new vertex_buffer();
  m_VBO->bind();
  m_VBO->set_data(data.vertices, data.vertex_size, GL_STATIC_DRAW);

  // Create and bind EBO
    m_EBO = new index_buffer();
  m_EBO->bind();
  m_EBO->set_data(data.indices, data.index_count * sizeof(unsigned int));

  // Set vertex attributes
  m_VAO->add_attributes(data.attributes);

  m_index_count = data.index_count;
}

void mesh_manager::bind() const {
  if (m_VAO) {
    m_VAO->bind();
  }
  if (m_EBO) {
    m_EBO->bind();
  }
}

void mesh_manager::draw() const {
  if (m_VAO && m_EBO && m_index_count > 0) {
    bind();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_index_count),
                   GL_UNSIGNED_INT, 0);
  }
}

