#pragma once

#include "input_handler.h"
#include "tests/cad/database/handle.h"
#include <glm/glm.hpp>
#include <optional>

#include <gp_Ax2.hxx>

namespace toy_cad::interaction {

enum class sketch_plane_source { none, brep_face, datum };

/** Result of picking a sketch support plane (datum or BREP face). */
struct sketch_plane_pick {
  sketch_plane_source source = sketch_plane_source::none;
  gp_Ax2 ax2{};
  handle source_handle{};
};

/**
 * Input handler: pick a datum plane or BREP face as sketch support.
 */
class sketch_plane_input_handler : public input_handler {
public:
  sketch_plane_input_handler() = default;

  sketch_plane_input_handler(const sketch_plane_input_handler &) = delete;
  sketch_plane_input_handler &operator=(const sketch_plane_input_handler &) =
      delete;
  sketch_plane_input_handler(sketch_plane_input_handler &&) noexcept = default;
  sketch_plane_input_handler &
  operator=(sketch_plane_input_handler &&) noexcept = default;

  ~sketch_plane_input_handler() override = default;

  bool on_mouse_moved(double xpos, double ypos) override;
  bool on_mouse_button(int button, int action, int mods) override;

  bool has_valid_hover() const {
    return m_hover_pick.source != sketch_plane_source::none;
  }
  const sketch_plane_pick &hover_pick() const {
    return m_hover_pick;
  }

private:
  sketch_plane_pick m_hover_pick{};

  void update_hover(double xpos, double ypos);
};

} // namespace toy_cad::interaction
