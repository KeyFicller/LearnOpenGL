#pragma once

#include "glm/glm.hpp"

namespace interaction_utils {

// Convert screen-space mouse coordinates to clip-space [-1, 1]x[-1, 1].
// Returns false if the current viewport is invalid (width/height <= 0).
bool screen_to_clip(double _xpos, double _ypos, glm::vec2 &_out_clip);

// Compute a vertical scale factor relative to a reference screen height
// using the current OpenGL viewport height.
// If the viewport height is invalid or ref height <= 0, returns 1.0f.
float viewport_scale_from_ref_height(float _ref_height);

} // namespace interaction_utils

