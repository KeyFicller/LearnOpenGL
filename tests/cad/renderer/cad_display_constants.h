#pragma once

namespace toy_cad {

/** Finite segment length for gp_Ax1 (conceptually infinite axis). World units. */
inline constexpr float k_infinite_axis_display_length = 1.12f;

/** Edge length of datum quad on gp_Ax2 plane (conceptually infinite plane). */
inline constexpr float k_infinite_datum_display_extent = 0.9f;

/** Cone base radius at axis segment end (world units). */
inline constexpr float k_axis_tip_cone_base_radius = 0.07f;

/** Cone height along axis past the segment end (world units). */
inline constexpr float k_axis_tip_cone_height = 0.15f;

} // namespace toy_cad
