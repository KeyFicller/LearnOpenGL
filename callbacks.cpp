#include "callbacks.h"

#include "tests/framework/test_suit.h"
#include <iostream>

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mode) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}

// GLFW error callback
void error_callback(int error, const char *description) {
  std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// GLFW framebuffer size callback
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);

  // Notify test suit about framebuffer resize
  test_suit *test_suit_ptr =
      static_cast<test_suit *>(glfwGetWindowUserPointer(window));
  if (test_suit_ptr) {
    test_suit_ptr->on_framebuffer_resized(width, height);
  }
}

// GLFW mouse cursor callback
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  // Forward mouse event to ImGui
  // ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

  // Process our camera movement
  test_suit *test_suit_ptr =
      static_cast<test_suit *>(glfwGetWindowUserPointer(window));
  if (test_suit_ptr) {
    auto real_xpos = xpos - test_suit_ptr->m_viewport_x;
    auto real_ypos = ypos - test_suit_ptr->m_viewport_y;
    test_suit_ptr->on_mouse_moved(real_xpos, real_ypos);

    // Check if the mouse is hovering over an object.
    // Viewport uses top-left origin (y down); framebuffer uses bottom-left (y
    // up).
    int fb_y = test_suit_ptr->m_scene_framebuffer->get_height() - 1 -
               static_cast<int>(real_ypos);
    auto object_id = test_suit_ptr->m_scene_framebuffer->read_object_id(
        static_cast<int>(real_xpos), fb_y);
    test_suit_ptr->on_object_hovered(object_id);
  }
}

// GLFW mouse button callback
void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  // Forward mouse button event to ImGui
  // ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

  // Process mouse button events here if needed
  test_suit *test_suit_ptr =
      static_cast<test_suit *>(glfwGetWindowUserPointer(window));
  if (test_suit_ptr) {
    test_suit_ptr->on_mouse_button(button, action, mods);
  }
}

// GLFW mouse scroll callback
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  // Forward scroll event to ImGui (it needs this for UI scrolling)
  // ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

  // Process our camera scroll (we'll check WantCaptureMouse in the scene logic)
  test_suit *test_suit_ptr =
      static_cast<test_suit *>(glfwGetWindowUserPointer(window));
  if (test_suit_ptr) {
    test_suit_ptr->on_mouse_scroll(xoffset, yoffset);
  }
}

// Process all input: query GLFW whether relevant keys are pressed/released,
// if they are we call corresponding functions
void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}
