#include "basic/vertex_array.h"

#include <glad/gl.h>
#include <stdexcept>
#include <vector>

// -----------------------------------------------------------------------------
void vertex_array::add_attributes(
    const std::vector<vertex_attribute> &_attributes) {
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
  for (size_t i = 0; i < _attributes.size(); i++) {
    glEnableVertexAttribArray(i);
    glVertexAttribPointer(i, _attributes[i].size, _attributes[i].type,
                          _attributes[i].normalized, total_stride,
                          (void *)attribute_offsets[i]);
  }
  unbind();
}



