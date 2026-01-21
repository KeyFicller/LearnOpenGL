#include "basic/texture.h"

#include "glad/gl.h"
#include "imgui.h"
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {
void apply_wrap_mode_impl(unsigned int _texture_type, unsigned int _texture_id,
                          wrap_mode _wrap_mode) {
  glBindTexture(_texture_type, _texture_id);
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
  glTexParameteri(_texture_type, GL_TEXTURE_WRAP_S, wrap_mode_gl);
  glTexParameteri(_texture_type, GL_TEXTURE_WRAP_T, wrap_mode_gl);
}

void apply_filter_mode_impl(unsigned int _texture_type,
                            unsigned int _texture_id,
                            filter_mode _filter_mode) {
  glBindTexture(_texture_type, _texture_id);
  GLint filter_mode_gl;
  switch (_filter_mode) {
  case filter_mode::k_nearest:
    filter_mode_gl = GL_NEAREST;
    break;
  case filter_mode::k_linear:
    filter_mode_gl = GL_LINEAR;
    break;
  }
  glTexParameteri(_texture_type, GL_TEXTURE_MIN_FILTER, filter_mode_gl);
  glTexParameteri(_texture_type, GL_TEXTURE_MAG_FILTER, filter_mode_gl);
}

} // namespace

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
  apply_wrap_mode_impl(GL_TEXTURE_2D, m_ID, _wrap_mode);
}

void texture_2d::set_filter_mode(filter_mode _filter_mode) {
  m_filter_mode = _filter_mode;
  apply_filter_mode_impl(GL_TEXTURE_2D, m_ID, _filter_mode);
}

// -----------------------------------------------------------------------------
void ui(texture_2d &_texture) {
  // Display texture preview
  ImGui::Text("Texture Preview");
  ImGui::Image((void *)(intptr_t)_texture.ID(), ImVec2(200, 200),
               ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));

  // Display texture information
  ImGui::Separator();
  ImGui::Text("Texture Info");
  ImGui::Text("Width: %d", _texture.width());
  ImGui::Text("Height: %d", _texture.height());
  ImGui::Text("Channels: %d", _texture.channels());
  ImGui::Text("ID: %u", _texture.ID());

  // Wrap mode control
  ImGui::Separator();
  int wrap_mode_current = static_cast<int>(_texture.get_wrap_mode());
  const char *wrap_mode_names[] = {"Repeat", "Mirrored Repeat", "Clamp to Edge",
                                   "Clamp to Border"};
  if (ImGui::Combo("Wrap Mode", &wrap_mode_current, wrap_mode_names,
                   IM_ARRAYSIZE(wrap_mode_names))) {
    _texture.set_wrap_mode(static_cast<wrap_mode>(wrap_mode_current));
  }

  // Filter mode control
  int filter_mode_current = static_cast<int>(_texture.get_filter_mode());
  const char *filter_mode_names[] = {"Nearest", "Linear"};
  if (ImGui::Combo("Filter Mode", &filter_mode_current, filter_mode_names,
                   IM_ARRAYSIZE(filter_mode_names))) {
    _texture.set_filter_mode(static_cast<filter_mode>(filter_mode_current));
  }
}

texture_cube::texture_cube(const std::array<std::string, 6> &_paths,
                           wrap_mode _wrap_mode, filter_mode _filter_mode) {
  glGenTextures(1, &m_ID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);

  stbi_set_flip_vertically_on_load(false);

  int width, height, nr_channels;
  unsigned char *data;
  for (int i = 0; i < 6; i++) {
    data = stbi_load(_paths[i].c_str(), &width, &height, &nr_channels, 0);
    GLenum format = GL_RGB;
    if (nr_channels == 1)
      format = GL_RED;
    else if (nr_channels == 3)
      format = GL_RGB;
    else if (nr_channels == 4)
      format = GL_RGBA;
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height,
                 0, format, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
  }
  set_wrap_mode(_wrap_mode);
  set_filter_mode(_filter_mode);
}

texture_cube::~texture_cube() { glDeleteTextures(1, &m_ID); }

void texture_cube::bind(int _slot) const {
  glActiveTexture(GL_TEXTURE0 + _slot);
  glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);
}

void texture_cube::set_wrap_mode(wrap_mode _wrap_mode) {
  m_wrap_mode = _wrap_mode;
  apply_wrap_mode_impl(GL_TEXTURE_CUBE_MAP, m_ID, _wrap_mode);
}

void texture_cube::set_filter_mode(filter_mode _filter_mode) {
  m_filter_mode = _filter_mode;
  apply_filter_mode_impl(GL_TEXTURE_CUBE_MAP, m_ID, _filter_mode);
}

void ui(texture_cube &_texture) {
  // Display texture preview
  ImGui::Text("Texture Preview");
  ImGui::Image((void *)(intptr_t)_texture.ID(), ImVec2(200, 200),
               ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));

  // Display texture information
  ImGui::Separator();
  ImGui::Text("Texture Info");
  ImGui::Text("ID: %u", _texture.ID());

  // Wrap mode control
  ImGui::Separator();
  int wrap_mode_current = static_cast<int>(_texture.get_wrap_mode());
  const char *wrap_mode_names[] = {"Repeat", "Mirrored Repeat", "Clamp to Edge",
                                   "Clamp to Border"};
  if (ImGui::Combo("Wrap Mode", &wrap_mode_current, wrap_mode_names,
                   IM_ARRAYSIZE(wrap_mode_names))) {
    _texture.set_wrap_mode(static_cast<wrap_mode>(wrap_mode_current));
  }

  // Filter mode control
  int filter_mode_current = static_cast<int>(_texture.get_filter_mode());
  const char *filter_mode_names[] = {"Nearest", "Linear"};
  if (ImGui::Combo("Filter Mode", &filter_mode_current, filter_mode_names,
                   IM_ARRAYSIZE(filter_mode_names))) {
    _texture.set_filter_mode(static_cast<filter_mode>(filter_mode_current));
  }
}