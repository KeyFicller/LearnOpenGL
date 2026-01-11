#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>

texture_2d::texture_2d(const char *_path, wrap_mode _wrap_mode,
                       filter_mode _filter_mode)
    : m_wrap_mode(_wrap_mode), m_filter_mode(_filter_mode) {
  stbi_set_flip_vertically_on_load(true);

  unsigned char *data =
      stbi_load(_path, &m_width, &m_height, &m_nr_channels, 0);
  if (!data) {
    std::cerr << "Failed to load texture: " << _path << std::endl;
    throw std::runtime_error("Failed to load texture");
  }

  glGenTextures(1, &m_ID);
  glBindTexture(GL_TEXTURE_2D, m_ID);

  GLenum format = GL_RGB;
  if (m_nr_channels == 1)
    format = GL_RED;
  else if (m_nr_channels == 3)
    format = GL_RGB;
  else if (m_nr_channels == 4)
    format = GL_RGBA;

  glTexImage2D(GL_TEXTURE_2D, 0, format, m_width, m_height, 0, format,
               GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(data);

  set_wrap_mode(_wrap_mode);
  set_filter_mode(_filter_mode);
}

texture_2d::~texture_2d() { glDeleteTextures(1, &m_ID); }

void texture_2d::bind(int _slot) const {
  glActiveTexture(GL_TEXTURE0 + _slot);
  glBindTexture(GL_TEXTURE_2D, m_ID);
}

void texture_2d::set_wrap_mode(wrap_mode _wrap_mode) {
  m_wrap_mode = _wrap_mode;
  glBindTexture(GL_TEXTURE_2D, m_ID);
  GLint wrap_mode_gl;
  switch (_wrap_mode) {
  case wrap_mode::k_repeat:
    wrap_mode_gl = GL_REPEAT;
    break;
  case wrap_mode::k_mirrored_repeat:
    wrap_mode_gl = GL_MIRRORED_REPEAT;
    break;
  case wrap_mode::k_clamp_to_edge:
    wrap_mode_gl = GL_CLAMP_TO_EDGE;
    break;
  case wrap_mode::k_clamp_to_border:
    wrap_mode_gl = GL_CLAMP_TO_BORDER;
    break;
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_mode_gl);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_mode_gl);
}

void texture_2d::set_filter_mode(filter_mode _filter_mode) {
  m_filter_mode = _filter_mode;
  glBindTexture(GL_TEXTURE_2D, m_ID);
  GLint filter_mode_gl;
  switch (_filter_mode) {
  case filter_mode::k_nearest:
    filter_mode_gl = GL_NEAREST;
    break;
  case filter_mode::k_linear:
    filter_mode_gl = GL_LINEAR;
    break;
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_mode_gl);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_mode_gl);
}
