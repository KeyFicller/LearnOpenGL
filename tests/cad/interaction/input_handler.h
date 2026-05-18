#pragma once

namespace toy_cad::interaction {

// Forward declaration
class command;

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

  /** Get the owner command that created this handler. */
  [[nodiscard]] command *owner() const { return m_owner; }

  /** Set the owner command (called by command when pushing handler). */
  void set_owner(command *owner, const char *flag) {
    m_owner = owner;
    m_flag = flag;
  }

  /** Get the flag identifier for this handler (e.g., "p1", "p2"). */
  [[nodiscard]] const char *flag() const { return m_flag; }

protected:
  input_handler() = default;

private:
  command *m_owner = nullptr;
  const char *m_flag = "";
};

inline bool input_handler::on_mouse_moved(double, double) { return false; }

inline bool input_handler::on_mouse_scroll(double, double) { return false; }

inline bool input_handler::on_mouse_button(int, int, int) { return false; }

inline void input_handler::on_framebuffer_resized(int, int) {}

} // namespace toy_cad::interaction
