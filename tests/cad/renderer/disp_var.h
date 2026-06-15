#pragma once

#include <glm/glm.hpp>

namespace toy_cad {

/** Per-frame display / camera state for CAD draws (world pass vs HUD). */
class disp_var {
public:
  glm::vec3 camera_world_position{0.f};
  glm::mat4 view_matrix{1.f};
  /** Projection matching the buffer {@link render_width}×{@link render_height} (e.g. scene FBO). */
  glm::mat4 projection_matrix{1.f};

  /** Last world pass render target size in pixels (scene framebuffer when bound). */
  int render_width = 0;
  int render_height = 0;

  /** True when the scene camera uses orthographic projection (see toy_cad_scene). */
  bool orthographic = false;

  /** Clip = P×V; vertex shader uses `uMVP * vec4(world_pos,1)` with world-space geometry. */
  [[nodiscard]] glm::mat4 clip_from_world() const {
    return projection_matrix * view_matrix;
  }
};

} // namespace toy_cad
