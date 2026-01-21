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
    test_suit_ptr->on_mouse_moved(xpos, ypos);
  }
}

// GLFW mouse button callback
void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  // Forward mouse button event to ImGui
  // ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

  // Process mouse button events here if needed
  // test_suit *test_suit_ptr =
  //     static_cast<test_suit *>(glfwGetWindowUserPointer(window));
  // if (test_suit_ptr) {
  //   test_suit_ptr->on_mouse_button(button, action, mods);
  // }
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
