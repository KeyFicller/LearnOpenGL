#include "misc_functions.h"

#include <iostream>

void query_maximum_vertex_attributes() {
  GLint max_vertex_attribs;
  glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_vertex_attribs);
  std::cout << "Maximum vertex attributes: " << max_vertex_attribs << std::endl;
}