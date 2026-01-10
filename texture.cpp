#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>
#include <string>

Texture2D::Texture2D(const char *path, WrapMode wrapMode, FilterMode filterMode)
    : m_wrapMode(wrapMode), m_filterMode(filterMode) {
  // Flip texture vertically on load (OpenGL expects origin at bottom-left)
  stbi_set_flip_vertically_on_load(true);

  glGenTextures(1, &m_ID);
  glBindTexture(GL_TEXTURE_2D, m_ID);
  unsigned char *data = stbi_load(path, &m_width, &m_height, &m_nrChannels, 0);
  if (data) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0,
                 m_nrChannels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
  } else {
    throw std::runtime_error("Failed to load texture: " + std::string(path));
  }
}

Texture2D::~Texture2D() { glDeleteTextures(1, &m_ID); }

void Texture2D::bind(int slot) const {
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, m_ID);

  switch (m_wrapMode) {
  case WrapMode::Repeat:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    break;
  case WrapMode::MirroredRepeat:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    break;
  case WrapMode::ClampToEdge:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    break;
  case WrapMode::ClampToBorder:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    break;
  default:
    break;
  }

  switch (m_filterMode) {
  case FilterMode::Nearest:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    break;
  case FilterMode::Linear:
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    break;
  default:
    break;
  }
}

void Texture2D::set_wrap_mode(WrapMode wrapMode) { m_wrapMode = wrapMode; }

void Texture2D::set_filter_mode(FilterMode filterMode) {
  m_filterMode = filterMode;
}