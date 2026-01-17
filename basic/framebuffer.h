#pragma once

#include <glad/gl.h>

// Framebuffer class for rendering to texture
class framebuffer {
public:
  framebuffer(int _width, int _height);
  ~framebuffer();

  // Disable copy constructor and copy assignment
  framebuffer(const framebuffer &) = delete;
  framebuffer &operator=(const framebuffer &) = delete;

  // Resize framebuffer
  void resize(int _width, int _height);

  // Bind framebuffer for rendering
  void bind() const;

  // Unbind framebuffer (bind to default framebuffer 0)
  void unbind() const;

  // Get color texture ID for use with ImGui::Image
  unsigned int get_color_texture() const { return m_color_texture; }

  // Get framebuffer dimensions
  int get_width() const { return m_width; }
  int get_height() const { return m_height; }

  // Check if framebuffer is complete
  bool is_complete() const;

private:
  void create_framebuffer(int _width, int _height);
  void destroy_framebuffer();

  unsigned int m_FBO = 0;
  unsigned int m_color_texture = 0;
  unsigned int m_depth_stencil_RBO = 0;
  int m_width = 0;
  int m_height = 0;
};

