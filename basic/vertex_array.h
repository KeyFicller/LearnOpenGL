#pragma once

#include <glad/gl.h>

#include <cstddef>
#include <stdexcept>
#include <vector>

/**
 * @brief Vertex buffer object (VBO) wrapper
 */
class vertex_buffer {
public:
  vertex_buffer() { glGenBuffers(1, &m_ID); }
  virtual ~vertex_buffer() { glDeleteBuffers(1, &m_ID); }

public:
  /**
   * @brief Bind the buffer object
   */
  void bind() const { glBindBuffer(GL_ARRAY_BUFFER, m_ID); }

  /**
   * @brief Unbind the buffer object
   */
  void unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

  /**
   * @brief Set buffer data
   * @param data Pointer to data
   * @param size Size of data in bytes
   * @param usage Buffer usage hint (default: GL_STATIC_DRAW)
   */
  void set_data(const void *data, size_t size,
                GLenum usage = GL_STATIC_DRAW) const {
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
  }

  /**
   * @brief Get OpenGL buffer ID
   * @return Buffer ID
   */
  unsigned int ID() const { return m_ID; }

protected:
  unsigned int m_ID = -1;
};

/**
 * @brief Vertex attribute specification
 */
struct vertex_attribute {
  unsigned int size; // Number of components (1-4)
  unsigned int type; // Data type (GL_FLOAT, GL_INT, etc.)
  bool normalized;   // Whether values should be normalized
};

/**
 * @brief Vertex array object (VAO) wrapper
 */
class vertex_array {
public:
  vertex_array() { glGenVertexArrays(1, &m_ID); }
  virtual ~vertex_array() { glDeleteVertexArrays(1, &m_ID); }

public:
  /**
   * @brief Bind the vertex array object
   */
  void bind() const { glBindVertexArray(m_ID); }

  /**
   * @brief Unbind the vertex array object
   */
  void unbind() const { glBindVertexArray(0); }

  /**
   * @brief Configure vertex attributes
   * @param _attributes Vector of vertex attribute specifications
   */
  void add_attributes(const std::vector<vertex_attribute> &_attributes);

  /**
   * @brief Get OpenGL vertex array ID
   * @return Vertex array ID
   */
  unsigned int ID() const { return m_ID; }

protected:
  unsigned int m_ID = -1;
};

/**
 * @brief Index buffer object (EBO/IBO) wrapper
 */
class index_buffer {
public:
  index_buffer() { glGenBuffers(1, &m_ID); }
  virtual ~index_buffer() { glDeleteBuffers(1, &m_ID); }

public:
  /**
   * @brief Bind the buffer object
   */
  void bind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID); }

  /**
   * @brief Unbind the buffer object
   */
  void unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

  /**
   * @brief Set buffer data
   * @param data Pointer to data
   * @param size Size of data in bytes
   * @param usage Buffer usage hint (default: GL_STATIC_DRAW)
   */
  void set_data(const void *data, size_t size,
                GLenum usage = GL_STATIC_DRAW) const {
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);
  }

  /**
   * @brief Get OpenGL buffer ID
   * @return Buffer ID
   */
  unsigned int ID() const { return m_ID; }

protected:
  unsigned int m_ID = -1;
};



