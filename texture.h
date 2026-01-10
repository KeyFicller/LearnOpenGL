#pragma once

#include <glad/gl.h>

#include <GLFW/glfw3.h>

class Texture2D {
public:
  enum class WrapMode {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
  };
  enum class FilterMode {
    Nearest,
    Linear,
  };

public:
  Texture2D(const char *path, WrapMode wrapMode = WrapMode::Repeat,
            FilterMode filterMode = FilterMode::Nearest);
  virtual ~Texture2D();

  Texture2D(const Texture2D &) = delete;
  Texture2D &operator=(const Texture2D &) = delete;
  Texture2D(Texture2D &&) = delete;
  Texture2D &operator=(Texture2D &&) = delete;

public:
  void bind(int slot = 0) const;
  void set_wrap_mode(WrapMode wrapMode);
  void set_filter_mode(FilterMode filterMode);

protected:
  unsigned int m_ID = -1;
  WrapMode m_wrapMode = WrapMode::Repeat;
  FilterMode m_filterMode = FilterMode::Nearest;
  int m_width = 0;
  int m_height = 0;
  int m_nrChannels = 0;
};