#include "basic/framebuffer.h"

#include <iostream>

framebuffer::framebuffer(int _width, int _height)
    : m_width(_width), m_height(_height) {
  create_framebuffer(_width, _height);
}

framebuffer::~framebuffer() { destroy_framebuffer(); }

void framebuffer::create_framebuffer(int _width, int _height) {
  m_width = _width;
  m_height = _height;

  // Create framebuffer object
  glGenFramebuffers(1, &m_FBO);
  glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

  // Create color texture
  glGenTextures(1, &m_color_texture);
  glBindTexture(GL_TEXTURE_2D, m_color_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _width, _height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         m_color_texture, 0);

  // Create object ID texture (one unsigned int per pixel, for picking)
  glGenTextures(1, &m_object_id_texture);
  glBindTexture(GL_TEXTURE_2D, m_object_id_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, _width, _height, 0, GL_RED_INTEGER,
               GL_UNSIGNED_INT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                         m_object_id_texture, 0);

  // Draw to both color and object ID attachments
  const GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  glDrawBuffers(2, draw_buffers);

  // Create depth and stencil renderbuffer
  glGenRenderbuffers(1, &m_depth_stencil_RBO);
  glBindRenderbuffer(GL_RENDERBUFFER, m_depth_stencil_RBO);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, _width, _height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, m_depth_stencil_RBO);

  // Check if framebuffer is complete
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!"
              << std::endl;
  }

  // Unbind framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void framebuffer::destroy_framebuffer() {
  if (m_FBO != 0) {
    glDeleteFramebuffers(1, &m_FBO);
    m_FBO = 0;
  }
  if (m_color_texture != 0) {
    glDeleteTextures(1, &m_color_texture);
    m_color_texture = 0;
  }
  if (m_object_id_texture != 0) {
    glDeleteTextures(1, &m_object_id_texture);
    m_object_id_texture = 0;
  }
  if (m_depth_stencil_RBO != 0) {
    glDeleteRenderbuffers(1, &m_depth_stencil_RBO);
    m_depth_stencil_RBO = 0;
  }
}

void framebuffer::resize(int _width, int _height) {
  if (m_width == _width && m_height == _height) {
    return; // No need to resize
  }

  destroy_framebuffer();
  create_framebuffer(_width, _height);
}

void framebuffer::bind() const {
  glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
  glViewport(0, 0, m_width, m_height);
}

void framebuffer::unbind() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

bool framebuffer::is_complete() const {
  glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
  bool complete =
      (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return complete;
}

int framebuffer::read_object_id(int x, int y) const {
  if (x < 0 || x >= m_width || y < 0 || y >= m_height)
    return -1;
  glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
  glReadBuffer(GL_COLOR_ATTACHMENT1);
  unsigned int id = 0;
  glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, &id);
  glReadBuffer(GL_COLOR_ATTACHMENT0); // restore default read buffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return (int)id;
}

void framebuffer::reset_object_id_texture() {
  // FBO must be bound when calling; we do not unbind so rendering can continue
  // to this FBO
  const unsigned int invalid_id = 0xFFFFFFFFu; // -1 as unsigned (no object)
  glClearBufferuiv(GL_COLOR, 1, &invalid_id);
}
