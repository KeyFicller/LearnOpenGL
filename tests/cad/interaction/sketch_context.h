#pragma once

#include "basic/camera.h"
#include "tests/cad/database/handle.h"
#include "tests/cad/interaction/sketch_plane_input_handler.h"

#include <functional>
#include <glm/glm.hpp>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <optional>

namespace toy_cad {

class sketch_feature;

/** Live sketch curve preview while a draw sub-command is active. */
struct sketch_live_preview {
  bool show_cursor = false;
  glm::vec3 cursor_world{0.0f};
  bool show_segment = false;
  gp_Pnt seg_p1{};
  gp_Pnt seg_p2{};
  bool show_circle = false;
  gp_Pnt circle_center{};
  double circle_radius = 0.0;
};

/** Active sketch session: plane, preview feature, camera restore, panel exit hook. */
class sketch_context {
public:
  static sketch_context &instance();

  sketch_context(const sketch_context &) = delete;
  sketch_context &operator=(const sketch_context &) = delete;

  [[nodiscard]] bool is_active() const { return m_active; }
  [[nodiscard]] const gp_Ax2 &plane() const { return m_plane; }
  [[nodiscard]] handle sketch_handle() const { return m_sketch_handle; }

  [[nodiscard]] sketch_feature *sketch();
  [[nodiscard]] const sketch_feature *sketch() const;

  void begin(const gp_Ax2 &plane, handle sketch_handle, camera &cam);
  void finish();
  void cancel();

  void apply_sketch_camera(camera &cam) const;
  void restore_camera(camera &cam) const;

  void set_on_finish(std::function<void()> fn) { m_on_finish = std::move(fn); }

  void set_live_preview(const sketch_live_preview &preview) {
    m_live_preview = preview;
  }
  void clear_live_preview() { m_live_preview = {}; }
  [[nodiscard]] const sketch_live_preview &live_preview() const {
    return m_live_preview;
  }

  void set_plane_hover(const std::optional<interaction::sketch_plane_pick> &pick) {
    m_plane_hover = pick;
  }
  [[nodiscard]] const std::optional<interaction::sketch_plane_pick> &
  plane_hover() const {
    return m_plane_hover;
  }

private:
  sketch_context() = default;

  struct camera_snapshot {
    glm::vec3 position{};
    glm::vec3 world_up{};
    glm::vec3 front{};
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool orthographic = false;
    float left = -1.0f;
    float right = 1.0f;
    float bottom = -1.0f;
    float top = 1.0f;
    float aspect_ratio = 1.0f;
    bool valid = false;
  };

  bool m_active = false;
  gp_Ax2 m_plane{};
  handle m_sketch_handle{};
  camera_snapshot m_saved_camera{};
  std::function<void()> m_on_finish{};
  sketch_live_preview m_live_preview{};
  std::optional<interaction::sketch_plane_pick> m_plane_hover{};

  static camera_snapshot capture_camera(const camera &cam);
  static void apply_snapshot(camera &cam, const camera_snapshot &snap);
};

} // namespace toy_cad
