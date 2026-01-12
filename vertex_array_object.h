#pragma once

#include <glad/gl.h>

#include <cstddef>
#include <vector>

class vertex_buffer_object {
public:
  vertex_buffer_object() { glGenBuffers(1, &m_ID); }
  virtual ~vertex_buffer_object() { glDeleteBuffers(1, &m_ID); }

public:
  void bind() const { glBindBuffer(GL_ARRAY_BUFFER, m_ID); }
  void unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }
  void set_data(const void *data, size_t size,
                GLenum usage = GL_STATIC_DRAW) const {
    glBufferData(GL_ARRAY_BUFFER, size, data, usage);
  }

protected:
  unsigned int m_ID = -1;
};

struct vertex_attribute {
  unsigned int size;
  unsigned int type;
  bool normalized;
};

class vertex_array_object {
public:
  vertex_array_object() { glGenVertexArrays(1, &m_ID); }
  virtual ~vertex_array_object() { glDeleteVertexArrays(1, &m_ID); }

public:
  void bind() const { glBindVertexArray(m_ID); }
  void unbind() const { glBindVertexArray(0); }
  void add_attributes(const std::vector<vertex_attribute> &_attributes) {
    bind();
    size_t total_stride = 0;
    std::vector<size_t> attribute_offsets;
    for (auto &attribute : _attributes) {
      attribute_offsets.push_back(total_stride);
      switch (attribute.type) {
      case GL_FLOAT:
        total_stride += attribute.size * sizeof(GLfloat);
        break;
      case GL_INT:
        total_stride += attribute.size * sizeof(GLint);
        break;
      default:
        throw std::runtime_error("Unsupported vertex attribute type");
      }
    }
    for (int i = 0; i < _attributes.size(); i++) {
      glEnableVertexAttribArray(i);
      glVertexAttribPointer(i, _attributes[i].size, _attributes[i].type,
                            _attributes[i].normalized, total_stride,
                            (void *)attribute_offsets[i]);
    }
    unbind();
  }

protected:
  unsigned int m_ID = -1;
};

class index_buffer_object {
public:
  index_buffer_object() { glGenBuffers(1, &m_ID); }
  virtual ~index_buffer_object() { glDeleteBuffers(1, &m_ID); }

public:
  void bind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID); }
  void unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }
  void set_data(const void *data, size_t size) const {
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
  }

protected:
  unsigned int m_ID = -1;
};
