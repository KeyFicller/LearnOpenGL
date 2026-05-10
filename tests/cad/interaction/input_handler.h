#pragma once

namespace toy_cad::interaction {

/** Top of `instance` input stack handles UI / interaction first. */
class input_handler {
public:
  input_handler(const input_handler &) = delete;
  input_handler &operator=(const input_handler &) = delete;
  input_handler(input_handler &&) noexcept = default;
  input_handler &operator=(input_handler &&) noexcept = default;

  virtual ~input_handler() = default;

  virtual bool on_mouse_moved(double xpos, double ypos);
  virtual bool on_mouse_scroll(double xoffset, double yoffset);
  virtual bool on_mouse_button(int button, int action, int mods);
  virtual void on_framebuffer_resized(int width, int height);

protected:
  input_handler() = default;
};

inline bool input_handler::on_mouse_moved(double, double) { return false; }

inline bool input_handler::on_mouse_scroll(double, double) { return false; }

inline bool input_handler::on_mouse_button(int, int, int) { return false; }

inline void input_handler::on_framebuffer_resized(int, int) {}

} // namespace toy_cad::interaction
