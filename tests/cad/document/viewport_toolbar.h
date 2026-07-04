#pragma once

#include <functional>

namespace toy_cad::interaction {

/**
 * Floating toolbar drawn on top of the Scene Viewport image (ImGui overlay).
 */
class viewport_toolbar {
public:
  static viewport_toolbar &instance();

  viewport_toolbar(const viewport_toolbar &) = delete;
  viewport_toolbar &operator=(const viewport_toolbar &) = delete;
  viewport_toolbar(viewport_toolbar &&) noexcept = default;
  viewport_toolbar &operator=(viewport_toolbar &&) noexcept = default;

  /** Draw inside the open "Scene Viewport" ImGui window. */
  void draw_in_viewport(std::function<void()> fit_view = {});

private:
  viewport_toolbar() = default;
  ~viewport_toolbar() = default;
};

} // namespace toy_cad::interaction
