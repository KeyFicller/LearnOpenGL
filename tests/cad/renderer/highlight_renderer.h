#pragma once

#include "tests/cad/geometry/subshape_handle.h"
#include "tests/cad/interaction/selection_manager.h"

#include <glm/glm.hpp>

namespace toy_cad::renderer {

/**
 * Renders highlight overlays for selected and hovered subshapes.
 * Uses stencil buffer or wireframe overlay for visibility.
 */
class highlight_renderer {
public:
  static highlight_renderer &instance();

  highlight_renderer(const highlight_renderer &) = delete;
  highlight_renderer &operator=(const highlight_renderer &) = delete;
  highlight_renderer(highlight_renderer &&) noexcept = default;
  highlight_renderer &operator=(highlight_renderer &&) noexcept = default;

  /**
   * Initialize OpenGL resources.
   */
  void init();

  /**
   * Cleanup OpenGL resources.
   */
  void cleanup();

  /**
   * Render highlights for all selected and hovered subshapes.
   * Should be called after the main geometry pass.
   */
  void render_highlights();

  /**
   * Render drag selection rectangle on the scene viewport (ImGui overlay).
   * Must be called inside the "Scene Viewport" ImGui window.
   */
  void render_drag_rect_in_viewport();

  void draw_screen_rect(const glm::vec2 &min_rect,
                       const glm::vec2 &max_rect,
                       const glm::vec4 &color);

  /**
   * Draw a 3D bounding box wireframe.
   * @param box The bounding box to draw
   * @param color Line color
   */
  void draw_bounding_box(const geometry::aabb &box, const glm::vec3 &color);

private:
  highlight_renderer() = default;
  ~highlight_renderer();

  // Shader program for highlight rendering
  unsigned int m_highlight_shader = 0;

  // VAO/VBO for screen-space quad (drag rect)
  unsigned int m_screen_quad_vao = 0;
  unsigned int m_screen_quad_vbo = 0;

  // VAO/VBO for wireframe bounding box
  unsigned int m_box_wire_vao = 0;
  unsigned int m_box_wire_vbo = 0;

  bool m_initialized = false;

  void init_shaders();
  void init_buffers();

  // Simple shader sources for unlit rendering
  static constexpr const char *k_highlight_vs = R"(#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

  static constexpr const char *k_highlight_fs = R"(#version 330 core
uniform vec3 uColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(uColor, 1.0);
}
)";

  static constexpr const char *k_screen_vs = R"(#version 330 core
layout(location = 0) in vec2 aPos;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

  static constexpr const char *k_screen_fs = R"(#version 330 core
uniform vec4 uColor;
out vec4 FragColor;
void main() {
    FragColor = uColor;
}
)";
};

} // namespace toy_cad::renderer
