#pragma once

#include "basic/camera.h"
#include "tests/cad/geometry/subshape_handle.h"

#include <gp_Ax2.hxx>
#include <optional>

namespace toy_cad {

/** Align camera to a top-down orthographic view of {@p plane}. */
void align_camera_to_sketch_plane(
    camera &cam, const gp_Ax2 &plane, float aspect_ratio,
    const std::optional<geometry::aabb> &fit_bounds = std::nullopt);

} // namespace toy_cad
