#pragma once

#include <glad/gl.h>

#include <GLFW/glfw3.h>

/**
 * @brief GLFW key callback
 */
void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mode);

/**
 * @brief GLFW error callback
 */
void error_callback(int error, const char *description);

/**
 * @brief GLFW framebuffer size callback
 */
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

/**
 * @brief GLFW mouse cursor position callback
 */
void mouse_callback(GLFWwindow *window, double xpos, double ypos);

/**
 * @brief GLFW mouse button callback
 */
void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods);

/**
 * @brief GLFW mouse scroll callback
 */
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

/**
 * @brief Process input (keyboard)
 */
void processInput(GLFWwindow *window);
