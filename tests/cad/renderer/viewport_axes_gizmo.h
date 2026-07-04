#pragma once

#include <glm/glm.hpp>

namespace toy_cad {

/** Mini axes HUD: delegates each plane/segment to shared {@link viewport_datum} / {@link viewport_axis} on {@link instance}. */
class viewport_axes_gizmo {
public:
  viewport_axes_gizmo() = default;
  ~viewport_axes_gizmo() = default;

  viewport_axes_gizmo(const viewport_axes_gizmo &) = delete;
  viewport_axes_gizmo &operator=(const viewport_axes_gizmo &) = delete;

  void draw(const glm::mat4 &view_matrix);

  /** Corner-style three translucent planes at fixed pixel size; center follows {@code world_anchor} projection (zoom-independent). */
  static void draw_datum_planes_screen_fixed(const glm::vec3 &world_anchor,
                                             const glm::mat4 &view_matrix,
                                             const glm::mat4 &clip_from_world);

  /** Corner-style RGB axes + cones + labels; same pixel sizing; anchored at {@code world_anchor} projection. */
  static void draw_axes_screen_fixed(const glm::vec3 &world_anchor,
                                     const glm::mat4 &view_matrix,
                                     const glm::mat4 &clip_from_world);
};

} // namespace toy_cad
