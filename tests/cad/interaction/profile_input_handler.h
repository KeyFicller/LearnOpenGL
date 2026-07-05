#pragma once

#include "input_handler.h"
#include "tests/cad/database/handle.h"

#include <glm/glm.hpp>
#include <optional>

#include <TopoDS_Face.hxx>

namespace toy_cad::interaction {

enum class profile_source_kind { none, face, sketch };

/** Result of picking an extrusion profile (BREP face or closed sketch). */
struct profile_pick {
  profile_source_kind source = profile_source_kind::none;
  TopoDS_Face profile_face;
  glm::vec3 direction{0.0f, 0.0f, 1.0f};
  glm::vec3 anchor{0.0f};
  handle face_handle{};
  handle sketch_handle{};
};

/**
 * Input handler for picking an extrusion profile: a BREP face or a closed sketch.
 */
class profile_input_handler : public input_handler {
public:
  profile_input_handler() = default;

  profile_input_handler(const profile_input_handler &) = delete;
  profile_input_handler &operator=(const profile_input_handler &) = delete;
  profile_input_handler(profile_input_handler &&) noexcept = default;
  profile_input_handler &operator=(profile_input_handler &&) noexcept = default;

  ~profile_input_handler() override = default;

  bool on_mouse_moved(double xpos, double ypos) override;
  bool on_mouse_button(int button, int action, int mods) override;

  bool has_valid_hover() const {
    return m_hover_pick.source != profile_source_kind::none;
  }
  const profile_pick &hover_pick() const { return m_hover_pick; }

private:
  profile_pick m_hover_pick{};

  void update_hover(double xpos, double ypos);
};

} // namespace toy_cad::interaction
