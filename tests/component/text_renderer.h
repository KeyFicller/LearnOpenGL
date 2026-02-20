#pragma once

#include "basic/shader.h"
#include <cstdint>
#include <ft2build.h>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <vector>
#include FT_FREETYPE_H

struct text_character {
  GLuint TextureID;
  glm::ivec2 Size;
  glm::ivec2 Bearing;
  GLuint Advance;
};

class text_renderer {
public:
  static text_renderer &instance() {
    static text_renderer ins;
    return ins;
  }

  ~text_renderer();

protected:
  text_renderer();

public:
  void set_font_size(int _pixel_size);
  /// Load glyph for Unicode codepoint (dynamic load). Supports ASCII and CJK.
  void load_character(std::uint32_t _codepoint);

  void render_text_by_pixel(const std::string &_text, float _x, float _y,
                            float _scale, const glm::vec3 &_color);

  /// Draw at (_x, _y) in NDC [-1, 1]. _y up. Uses viewport to convert glyph
  /// size/advance from pixels to NDC.
  void render_text_by_uv(const std::string &_text, float _x, float _y,
                         float _scale, const glm::vec3 &_color,
                         float _z_offset = -1.0f);

  /// Measure text in NDC for given scale. Out: width_ndc, height_ndc,
  /// bearing_y_ndc (baseline to top of first line). Use to center text at
  /// (cx, cy): origin_x = cx - width_ndc/2, baseline = cy - height_ndc/2 +
  /// bearing_y_ndc.
  void measure_text_ndc(const std::string &_text, float _scale,
                        float *_out_width_ndc, float *_out_height_ndc,
                        float *_out_bearing_y_ndc);

private:
  /// Decode UTF-8 string to Unicode code points (for ASCII + Chinese etc.).
  static void utf8_to_codepoints(const std::string &_utf8,
                                 std::vector<std::uint32_t> &_out);

  FT_Library m_ft = nullptr;
  FT_Face m_face = nullptr;
  std::map<std::uint32_t, text_character> m_characters;
  shader *m_shader = nullptr;
  GLuint m_VAO = 0;
  GLuint m_VBO = 0;
  bool m_initialized = false;
  float m_z_offset = -1.0f; // Default Z offset for text
};