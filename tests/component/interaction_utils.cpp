#include "interaction_utils.h"

#include "glad/gl.h"

namespace interaction_utils {

bool screen_to_clip(double _xpos, double _ypos, glm::vec2 &_out_clip) {
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  float viewport_width = static_cast<float>(viewport[2]);
  float viewport_height = static_cast<float>(viewport[3]);

  if (viewport_width <= 0.0f || viewport_height <= 0.0f) {
    return false;
  }

  float x_screen =
      static_cast<float>(_xpos) - static_cast<float>(viewport[0]);
  float y_screen =
      static_cast<float>(_ypos) - static_cast<float>(viewport[1]);

  float x_norm = x_screen / viewport_width;
  float y_norm = y_screen / viewport_height;

  float x_clip = 2.0f * x_norm - 1.0f;
  float y_clip = 1.0f - 2.0f * y_norm;

  _out_clip = glm::vec2(x_clip, y_clip);
  return true;
}

float viewport_scale_from_ref_height(float _ref_height) {
  if (_ref_height <= 0.0f) {
    return 1.0f;
  }

  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  if (viewport[3] <= 0) {
    return 1.0f;
  }

  return static_cast<float>(viewport[3]) / _ref_height;
}

} // namespace interaction_utils

