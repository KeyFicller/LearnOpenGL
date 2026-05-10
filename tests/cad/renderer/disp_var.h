#pragma once

#include <glm/glm.hpp>

namespace toy_cad {

class disp_var {
public:
  glm::vec3 camera_world_position{0.f};
  glm::mat4 view_matrix{1.f};
  glm::mat4 projection_matrix{1.f};
};

} // namespace toy_cad
