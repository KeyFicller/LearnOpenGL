// Wrapper file for imgui_impl_opengl3.cpp to ensure GLAD is included before
// imgui When using IMGUI_IMPL_OPENGL_LOADER_CUSTOM, imgui_impl_opengl3.cpp
// doesn't include any OpenGL headers, so we need to include GLAD first.

// Include GLAD first to provide OpenGL types and functions
#include <glad/gl.h>

// Now include the imgui implementation
// Note: We use a relative path that will be resolved by CMake's include
// directories
#include "imgui_impl_opengl3.cpp"
