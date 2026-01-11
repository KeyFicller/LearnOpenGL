#pragma once

#include <glad/gl.h>

#include <GLFW/glfw3.h>

class texture_2d {
public:
  enum class wrap_mode {
    k_repeat,
    k_mirrored_repeat,
    k_clamp_to_edge,
    k_clamp_to_border,
  };
  enum class filter_mode {
    k_nearest,
    k_linear,
  };

public:
  texture_2d(const char *_path, wrap_mode _wrap_mode = wrap_mode::k_repeat,
             filter_mode _filter_mode = filter_mode::k_nearest);
  virtual ~texture_2d();

  texture_2d(const texture_2d &) = delete;
  texture_2d &operator=(const texture_2d &) = delete;
  texture_2d(texture_2d &&) = delete;
  texture_2d &operator=(texture_2d &&) = delete;

public:
  void bind(int _slot = 0) const;
  void set_wrap_mode(wrap_mode _wrap_mode);
  void set_filter_mode(filter_mode _filter_mode);

protected:
  unsigned int m_ID = -1;
  wrap_mode m_wrap_mode = wrap_mode::k_repeat;
  filter_mode m_filter_mode = filter_mode::k_nearest;
  int m_width = 0;
  int m_height = 0;
  int m_nr_channels = 0;
};
