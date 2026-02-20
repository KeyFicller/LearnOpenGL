#include "text_renderer.h"

#include <glad/gl.h>
#include <iostream>

void text_renderer::utf8_to_codepoints(const std::string &_utf8,
                                       std::vector<std::uint32_t> &_out) {
  _out.clear();
  const unsigned char *p =
      reinterpret_cast<const unsigned char *>(_utf8.data());
  const unsigned char *end = p + _utf8.size();
  while (p < end) {
    std::uint32_t cp = 0;
    unsigned char b = *p++;
    if (b < 0x80) {
      cp = b;
    } else if (b < 0xE0) {
      if (p >= end)
        break;
      cp = ((b & 0x1F) << 6) | (*p++ & 0x3F);
    } else if (b < 0xF0) {
      if (p + 1 >= end)
        break;
      cp = ((b & 0x0F) << 12) | ((p[0] & 0x3F) << 6) | (p[1] & 0x3F);
      p += 2;
    } else {
      if (p + 2 >= end)
        break;
      cp = ((b & 0x07) << 18) | ((p[0] & 0x3F) << 12) | ((p[1] & 0x3F) << 6) |
           (p[2] & 0x3F);
      p += 3;
    }
    _out.push_back(cp);
  }
}

text_renderer::text_renderer() {
  if (FT_Init_FreeType(&m_ft)) {
    std::cerr << "Failed to initialize FreeType" << std::endl;
    return;
  }

  if (FT_New_Face(m_ft, "assets/fonts/Arial Unicode.ttf", 0, &m_face)) {
    std::cerr << "Failed to load font" << std::endl;
    FT_Done_FreeType(m_ft);
    m_ft = nullptr;
    return;
  }

  set_font_size(48);

  m_shader = new shader("shaders/text_renderer_test/vertex.shader",
                        "shaders/text_renderer_test/fragment.shader");

  glGenVertexArrays(1, &m_VAO);
  glGenBuffers(1, &m_VBO);
  glBindVertexArray(m_VAO);
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                        (GLvoid *)0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  m_initialized = true;
}

text_renderer::~text_renderer() {
  if (!m_initialized)
    return;
  for (auto &character : m_characters) {
    glDeleteTextures(1, &character.second.TextureID);
  }
  if (m_face) {
    FT_Done_Face(m_face);
    m_face = nullptr;
  }
  if (m_ft) {
    FT_Done_FreeType(m_ft);
    m_ft = nullptr;
  }
  if (m_shader) {
    delete m_shader;
    m_shader = nullptr;
  }
  if (m_VAO) {
    glDeleteVertexArrays(1, &m_VAO);
    m_VAO = 0;
  }
  if (m_VBO) {
    glDeleteBuffers(1, &m_VBO);
    m_VBO = 0;
  }
}

void text_renderer::set_font_size(int _pixel_size) {
  if (!m_face)
    return;
  FT_Set_Pixel_Sizes(m_face, 0, _pixel_size);
}

void text_renderer::load_character(std::uint32_t _codepoint) {
  if (!m_face)
    return;
  if (m_characters.find(_codepoint) != m_characters.end())
    return; // already loaded
  if (FT_Load_Char(m_face, static_cast<FT_ULong>(_codepoint), FT_LOAD_RENDER)) {
    return; // font may not have this glyph (e.g. replacement box)
  }
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_face->glyph->bitmap.width,
               m_face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
               m_face->glyph->bitmap.buffer);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  text_character character = {
      texture,
      {m_face->glyph->bitmap.width, m_face->glyph->bitmap.rows},
      {m_face->glyph->bitmap_left, m_face->glyph->bitmap_top},
      static_cast<GLuint>(m_face->glyph->advance.x)};
  m_characters[_codepoint] = character;
}

void text_renderer::render_text_by_pixel(const std::string &_text, float _x,
                                         float _y, float _scale,
                                         const glm::vec3 &_color) {
  if (!m_initialized || !m_shader)
    return;

  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  float vp_w = static_cast<float>(viewport[2]);
  float vp_h = static_cast<float>(viewport[3]);
  if (vp_w <= 0 || vp_h <= 0)
    return;

  // Pixel (0,0) = top-left -> NDC (-1, 1); pixel (vp_w, vp_h) -> NDC (1, -1)
  auto to_ndc_x = [vp_w](float px) { return 2.0f * px / vp_w - 1.0f; };
  auto to_ndc_y = [vp_h](float py) { return 1.0f - 2.0f * py / vp_h; };

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  m_shader->use();
  m_shader->set_uniform("textColor", _color);
  m_shader->set_uniform("text", 0);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(m_VAO);

  std::vector<std::uint32_t> codepoints;
  utf8_to_codepoints(_text, codepoints);
  for (std::uint32_t cp : codepoints) {
    load_character(cp);
    auto it = m_characters.find(cp);
    if (it == m_characters.end())
      continue;
    const text_character &ch = it->second;

    float xpos = _x + ch.Bearing.x * _scale;
    float ypos = _y - ch.Bearing.y * _scale;
    float w = ch.Size.x * _scale;
    float h = ch.Size.y * _scale;

    GLfloat vertices[6][4] = {
        {to_ndc_x(xpos), to_ndc_y(ypos + h), 0.0, 1.0},
        {to_ndc_x(xpos), to_ndc_y(ypos), 0.0, 0.0},
        {to_ndc_x(xpos + w), to_ndc_y(ypos), 1.0, 0.0},

        {to_ndc_x(xpos), to_ndc_y(ypos + h), 0.0, 1.0},
        {to_ndc_x(xpos + w), to_ndc_y(ypos), 1.0, 0.0},
        {to_ndc_x(xpos + w), to_ndc_y(ypos + h), 1.0, 1.0}};
    glBindTexture(GL_TEXTURE_2D, ch.TextureID);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_shader->set_uniform("z_offset", m_z_offset);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    _x += (ch.Advance >> 6) * _scale;
  }

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_BLEND);
}

// Draw text at (_x, _y) in NDC [-1, 1]. _x,_y = baseline/origin; y axis up.
// Glyph size/advance are converted from pixels to NDC using current viewport.
void text_renderer::render_text_by_uv(const std::string &_text, float _x,
                                      float _y, float _scale,
                                      const glm::vec3 &_color,
                                      float _z_offset) {
  if (!m_initialized || !m_shader)
    return;

  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  float vp_w = static_cast<float>(viewport[2]);
  float vp_h = static_cast<float>(viewport[3]);
  if (vp_w <= 0 || vp_h <= 0)
    return;

  // 1 pixel -> NDC
  float px_to_ndc_x = 2.0f / vp_w;
  float px_to_ndc_y = 2.0f / vp_h;

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  m_shader->use();
  m_shader->set_uniform("textColor", _color);
  m_shader->set_uniform("text", 0);
  glActiveTexture(GL_TEXTURE0);
  glBindVertexArray(m_VAO);

  std::vector<std::uint32_t> codepoints;
  utf8_to_codepoints(_text, codepoints);
  for (std::uint32_t cp : codepoints) {
    load_character(cp);
    auto it = m_characters.find(cp);
    if (it == m_characters.end())
      continue;
    const text_character &ch = it->second;

    float xpos = _x + ch.Bearing.x * _scale * px_to_ndc_x;
    float top_ndc = _y + ch.Bearing.y * _scale * px_to_ndc_y;
    float w = ch.Size.x * _scale * px_to_ndc_x;
    float h = ch.Size.y * _scale * px_to_ndc_y;
    float bottom_ndc = top_ndc - h;

    GLfloat vertices[6][4] = {
        {xpos, bottom_ndc, 0.0, 1.0},    {xpos, top_ndc, 0.0, 0.0},
        {xpos + w, top_ndc, 1.0, 0.0},

        {xpos, bottom_ndc, 0.0, 1.0},    {xpos + w, top_ndc, 1.0, 0.0},
        {xpos + w, bottom_ndc, 1.0, 1.0}};

    glBindTexture(GL_TEXTURE_2D, ch.TextureID);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    m_shader->set_uniform("z_offset", _z_offset);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    _x += (ch.Advance >> 6) * _scale * px_to_ndc_x;
  }

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_BLEND);
}

void text_renderer::measure_text_ndc(const std::string &_text, float _scale,
                                     float *_out_width_ndc,
                                     float *_out_height_ndc,
                                     float *_out_bearing_y_ndc) {
  if (!_out_width_ndc && !_out_height_ndc && !_out_bearing_y_ndc)
    return;
  if (!m_face) {
    if (_out_width_ndc)
      *_out_width_ndc = 0;
    if (_out_height_ndc)
      *_out_height_ndc = 0;
    if (_out_bearing_y_ndc)
      *_out_bearing_y_ndc = 0;
    return;
  }
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  float vp_w = static_cast<float>(viewport[2]);
  float vp_h = static_cast<float>(viewport[3]);
  if (vp_w <= 0 || vp_h <= 0) {
    if (_out_width_ndc)
      *_out_width_ndc = 0;
    if (_out_height_ndc)
      *_out_height_ndc = 0;
    if (_out_bearing_y_ndc)
      *_out_bearing_y_ndc = 0;
    return;
  }
  float px_to_ndc_x = 2.0f / vp_w;
  float px_to_ndc_y = 2.0f / vp_h;

  std::vector<std::uint32_t> codepoints;
  utf8_to_codepoints(_text, codepoints);
  float width_ndc = 0;
  float height_ndc = 0;
  float bearing_y_ndc = 0;
  bool first = true;
  for (std::uint32_t cp : codepoints) {
    load_character(cp);
    auto it = m_characters.find(cp);
    if (it == m_characters.end())
      continue;
    const text_character &ch = it->second;
    width_ndc += (ch.Advance >> 6) * _scale * px_to_ndc_x;
    float h = ch.Size.y * _scale * px_to_ndc_y;
    if (h > height_ndc)
      height_ndc = h;
    if (first) {
      bearing_y_ndc = ch.Bearing.y * _scale * px_to_ndc_y;
      first = false;
    }
  }
  if (_out_width_ndc)
    *_out_width_ndc = width_ndc;
  if (_out_height_ndc)
    *_out_height_ndc = height_ndc;
  if (_out_bearing_y_ndc)
    *_out_bearing_y_ndc = bearing_y_ndc;
}