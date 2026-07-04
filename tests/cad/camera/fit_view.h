#pragma once

#include "basic/camera.h"
#include "tests/cad/geometry/subshape_handle.h"

namespace toy_cad {

/** Frame {@p cam} so {@p bounds} is fully visible, preserving yaw/pitch. */
void fit_camera_to_bounds(camera &cam, const geometry::aabb &bounds,
                          float aspect_ratio);

} // namespace toy_cad
