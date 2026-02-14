/**
 * @brief ImGui OpenGL3 implementation with GLAD loader
 *
 * When using IMGUI_IMPL_OPENGL_LOADER_CUSTOM, imgui_impl_opengl3.cpp doesn't
 * include any OpenGL headers, so we need to include GLAD first.
 *
 * This file ensures GLAD is included before ImGui's OpenGL3 backend
 * implementation.
 */

// Include GLAD first to provide OpenGL types and functions
#include <glad/gl.h>

// Now include the ImGui OpenGL3 implementation
#include "imgui_impl_opengl3.cpp"

// Now include the ImGui stdlib
#include "misc/cpp/imgui_stdlib.cpp"

// Now include the ImGuiColorTextEdit implementation
#include "TextEditor.cpp"
