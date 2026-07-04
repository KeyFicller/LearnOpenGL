#pragma once

#include "basic/camera.h"

#include <gp_Ax2.hxx>

namespace toy_cad {

/** Align camera to a top-down orthographic view of {@p plane}. */
void align_camera_to_sketch_plane(camera &cam, const gp_Ax2 &plane,
                                  float aspect_ratio);

} // namespace toy_cad
