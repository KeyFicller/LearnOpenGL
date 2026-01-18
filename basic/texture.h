#pragma once

#include <glad/gl.h>

#include <GLFW/glfw3.h>

/**
 * @brief 2D texture wrapper
 */
class texture_2d {
public:
  /**
   * @brief Texture wrap mode
   */
  enum class wrap_mode {
    k_repeat,
    k_mirrored_repeat,
    k_clamp_to_edge,
    k_clamp_to_border,
  };

  /**
   * @brief Texture filter mode
   */
  enum class filter_mode {
    k_nearest,
    k_linear,
  };

public:
  /**
   * @brief Load texture from file
   * @param _path Path to texture file
   * @param _wrap_mode Texture wrap mode
   * @param _filter_mode Texture filter mode
   */
  texture_2d(const char *_path, wrap_mode _wrap_mode = wrap_mode::k_repeat,
             filter_mode _filter_mode = filter_mode::k_nearest);
  virtual ~texture_2d();

  texture_2d(const texture_2d &) = delete;
  texture_2d &operator=(const texture_2d &) = delete;
  texture_2d(texture_2d &&) = delete;
  texture_2d &operator=(texture_2d &&) = delete;

public:
  /**
   * @brief Bind texture to texture unit
   * @param _slot Texture unit slot (default: 0)
   */
  void bind(int _slot = 0) const;

  /**
   * @brief Set texture wrap mode
   * @param _wrap_mode Wrap mode to set
   */
  void set_wrap_mode(wrap_mode _wrap_mode);

  /**
   * @brief Set texture filter mode
   * @param _filter_mode Filter mode to set
   */
  void set_filter_mode(filter_mode _filter_mode);

public:
  /**
   * @brief Get OpenGL texture ID
   * @return Texture ID
   */
  unsigned int ID() const { return m_ID; }

  /**
   * @brief Get texture width
   * @return Texture width in pixels
   */
  int width() const { return m_width; }

  /**
   * @brief Get texture height
   * @return Texture height in pixels
   */
  int height() const { return m_height; }

  /**
   * @brief Get number of channels
   * @return Number of color channels
   */
  int channels() const { return m_nr_channels; }

  /**
   * @brief Get current wrap mode
   * @return Current wrap mode
   */
  wrap_mode get_wrap_mode() const { return m_wrap_mode; }

  /**
   * @brief Get current filter mode
   * @return Current filter mode
   */
  filter_mode get_filter_mode() const { return m_filter_mode; }

protected:
  unsigned int m_ID = -1;
  wrap_mode m_wrap_mode = wrap_mode::k_repeat;
  filter_mode m_filter_mode = filter_mode::k_nearest;
  int m_width = 0;
  int m_height = 0;
  int m_nr_channels = 0;
};

// -----------------------------------------------------------------------------
/**
 * @brief UI for a texture
 * @param _texture The texture to display in the UI
 */
void ui(texture_2d &_texture);
