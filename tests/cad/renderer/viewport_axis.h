#pragma once

#include <cstdint>

#include <glm/glm.hpp>

class shader;
class vertex_array;
class vertex_buffer;

/** Global singleton: axis segment, tip cone, optional ImGui label. */
class viewport_axis {
public:
  static viewport_axis &instance();

  viewport_axis(const viewport_axis &) = delete;
  viewport_axis &operator=(const viewport_axis &) = delete;

  static constexpr float k_axis_len = 1.12f;

  void draw_lines(shader &shader_prog, const glm::mat4 &mvp,
                  const glm::vec3 &p0, const glm::vec3 &p1,
                  const glm::vec4 &color);

  /** Right circular cone: base centered at `tip` in plane ⊥ `dir_unit`, apex at
   *  `tip + cone_height * dir_unit` (arrow past axis segment end). */
  void draw_axis_tip(shader &shader_prog, const glm::mat4 &mvp,
                      const glm::vec3 &tip, const glm::vec3 &dir_unit,
                      float base_radius, float cone_height,
                      const glm::vec4 &color);

  /** Map `tip` through `mvp`; skip if label AABB falls outside Scene Viewport. */
  void draw_label(const glm::mat4 &mvp, int fbo_w, int fbo_h, int sub_x,
                  int sub_y, int sub_w, int sub_h, const glm::vec3 &tip,
                  const char *text, std::uint32_t color_abgr);

  ~viewport_axis();

private:
  viewport_axis();

  vertex_array *m_line_vao = nullptr;
  vertex_buffer *m_line_vbo = nullptr;
  vertex_array *m_tip_vao = nullptr;
  vertex_buffer *m_tip_vbo = nullptr;
};
