#pragma once

#include <algorithm>
#include <cmath>

struct GLFWwindow;

/// One frame of host input content forwarded to an attached C# script.
///
/// The scene records raw mouse events into this object, samples keyboard axes
/// each tick, then calls forward_* helpers so the script's HandleInput owns the
/// values. Game/state data stays in the .cs file; this class only carries input.
class script_input_content {
public:
  void on_mouse_moved(float xpos, float ypos) {
    m_cursor_x = xpos;
    m_cursor_y = ypos;
  }

  void on_mouse_button(int button, int action);

  /// Sample WASD/arrows (+ R / Space edges) and convert cursor pixels to clip NDC.
  /// When imgui_wants_keyboard is true, axes and key edges are cleared / ignored.
  void sample(GLFWwindow *window, int viewport_w, int viewport_h,
              bool imgui_wants_keyboard);

  float move_x() const { return m_move_x; }
  float move_y() const { return m_move_y; }
  float mouse_ndc_x() const { return m_mouse_ndc_x; }
  float mouse_ndc_y() const { return m_mouse_ndc_y; }

  /// bit0 = left held, bit1 = right held, bit2 = Space held
  int mouse_buttons() const {
    int bits = 0;
    if (m_left_down)
      bits |= 1;
    if (m_right_down)
      bits |= 2;
    if (m_space_down)
      bits |= 4;
    return bits;
  }

  /// bit0 = left pressed this frame, bit1 = right pressed, bit2 = Space pressed
  int mouse_clicked() const {
    int bits = 0;
    if (m_left_clicked)
      bits |= 1;
    if (m_right_clicked)
      bits |= 2;
    if (m_space_clicked)
      bits |= 4;
    return bits;
  }

  /// True once when R is pressed (edge), unless ImGui wants keyboard.
  bool reset_pressed() const { return m_reset_pressed; }

  /// Clear one-shot edges after they have been forwarded to C#.
  void clear_frame_edges() {
    m_left_clicked = false;
    m_right_clicked = false;
    m_space_clicked = false;
    m_reset_pressed = false;
  }

private:
  void sample_keyboard_axis(GLFWwindow *window, bool imgui_wants_keyboard);
  void cursor_to_ndc(int viewport_w, int viewport_h);

  float m_cursor_x = 0.0f;
  float m_cursor_y = 0.0f;
  bool m_left_down = false;
  bool m_right_down = false;
  bool m_left_clicked = false;
  bool m_right_clicked = false;

  float m_move_x = 0.0f;
  float m_move_y = 0.0f;
  float m_mouse_ndc_x = 0.0f;
  float m_mouse_ndc_y = 0.0f;

  bool m_r_was_down = false;
  bool m_reset_pressed = false;

  bool m_space_down = false;
  bool m_space_was_down = false;
  bool m_space_clicked = false;
};
