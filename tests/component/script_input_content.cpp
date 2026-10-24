#include "script_input_content.h"

#include <GLFW/glfw3.h>

void script_input_content::on_mouse_button(int button, int action) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      m_left_down = true;
      m_left_clicked = true;
    } else if (action == GLFW_RELEASE) {
      m_left_down = false;
    }
  } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (action == GLFW_PRESS) {
      m_right_down = true;
      m_right_clicked = true;
    } else if (action == GLFW_RELEASE) {
      m_right_down = false;
    }
  }
}

void script_input_content::sample_keyboard_axis(GLFWwindow *window,
                                                bool imgui_wants_keyboard) {
  m_move_x = 0.0f;
  m_move_y = 0.0f;
  if (!window || imgui_wants_keyboard)
    return;

  const bool left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
                    glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
  const bool right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
                     glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;
  const bool down = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
                    glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
  const bool up = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
                  glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;

  if (left)
    m_move_x -= 1.0f;
  if (right)
    m_move_x += 1.0f;
  if (down)
    m_move_y -= 1.0f;
  if (up)
    m_move_y += 1.0f;

  const float len2 = m_move_x * m_move_x + m_move_y * m_move_y;
  if (len2 > 1.0f) {
    const float inv = 1.0f / std::sqrt(len2);
    m_move_x *= inv;
    m_move_y *= inv;
  }
}

void script_input_content::cursor_to_ndc(int viewport_w, int viewport_h) {
  m_mouse_ndc_x = 0.0f;
  m_mouse_ndc_y = 0.0f;
  if (viewport_w <= 0 || viewport_h <= 0)
    return;
  // Viewport mouse is top-left origin; clip NDC is y-up.
  m_mouse_ndc_x =
      (m_cursor_x / static_cast<float>(viewport_w)) * 2.0f - 1.0f;
  m_mouse_ndc_y =
      1.0f - (m_cursor_y / static_cast<float>(viewport_h)) * 2.0f;
  m_mouse_ndc_x = std::clamp(m_mouse_ndc_x, -1.0f, 1.0f);
  m_mouse_ndc_y = std::clamp(m_mouse_ndc_y, -1.0f, 1.0f);
}

void script_input_content::sample(GLFWwindow *window, int viewport_w,
                                  int viewport_h, bool imgui_wants_keyboard) {
  sample_keyboard_axis(window, imgui_wants_keyboard);
  cursor_to_ndc(viewport_w, viewport_h);

  const bool keys_ok = window && !imgui_wants_keyboard;

  const bool r_down =
      keys_ok && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
  m_reset_pressed = r_down && !m_r_was_down;
  m_r_was_down = r_down;

  // Space is packed into mouse_buttons/clicked bit2 so Input.SetFrame arity
  // stays stable; C# can read it via Input.ButtonHeld/Down(2).
  m_space_down =
      keys_ok && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
  m_space_clicked = m_space_down && !m_space_was_down;
  m_space_was_down = m_space_down;
}
