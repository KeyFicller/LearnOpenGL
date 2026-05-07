#pragma once

#include "imgui.h"

#include <algorithm>

/**
 * Width for the value widget on a labeled row (slider, combo, color edit).
 * Uses a fraction of GetContentRegionAvail().x (after indent), clamped so rows
 * stay readable on both narrow and very wide panels.
 */
inline float imgui_form_item_width(float line_fraction = 0.72f, float min_px = 120.0f,
                                   float max_px = 400.0f) {
  const float avail = ImGui::GetContentRegionAvail().x;
  return std::clamp(avail * line_fraction, min_px, max_px);
}
