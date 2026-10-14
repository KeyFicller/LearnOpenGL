#pragma once

// MCP server for the live shader editor. Tools are ordinary members; the base
// class publishes them by reflection. Calls that touch the editor or GL are
// forwarded to shader_editor_scene, which runs them on the render thread.

#include "llm/reflection_mcp.h"

#include <thread>

class shader_editor_scene;

inline constexpr int k_shader_editor_mcp_port = 8765;

class shader_editor_mcp : public mcp::Server<shader_editor_mcp> {
public:
  explicit shader_editor_mcp(shader_editor_scene *scene);
  ~shader_editor_mcp() override;

  void start();

  [[= mcp::tool{.read_only = true, .open_world = false}]][[= js::doc{
      .text = js::str("Reads the shader project currently open in the editor, "
                      "including both sources.")}]] std::string
  get_shader();

  [[= mcp::tool{.read_only = false,
                .destructive = false,
                .idempotent = true,
                .open_world = false}]][[= js::doc{
      .text = js::str(
          "Replaces the open project's vertex and/or fragment shader, saves "
          "it, and compiles it. An empty string leaves that stage unchanged. "
          "The viewport shows the result on the next frame.")}]][[= js::param_docs(
      js::str("Full vertex shader source, or empty to keep the current one."),
      js::str(
          "Full fragment shader source, or empty to keep the current one."))]]
  std::string
  set_shader(std::string vertex, std::string fragment);

  [[= mcp::tool{.read_only = true, .open_world = false}]][[= js::doc{
      .text = js::str("Lists shader projects and which one is open.")}]]
  std::string
  list_projects();

  [[= mcp::tool{.read_only = false,
                .destructive = false,
                .idempotent = true,
                .open_world = false}]][[= js::doc{
      .text = js::str(
          "Saves the open project and switches to another one on disk.")}]]
  [[= js::param_docs(js::str("Project folder name under shaders/shader_editor_test."))]]
  std::string
  open_project(std::string name);

  [[= mcp::tool{.read_only = true, .open_world = false}]][[= js::doc{
      .text = js::str(
          "The uniforms the engine fills every frame, with their GLSL types, "
          "what they mean, and their current values. Declare these names in "
          "the shader; do not invent others.")}]] std::string
  builtin_uniforms();

  [[= mcp::tool{.read_only = true, .open_world = false}]][[= js::doc{
      .text = js::str(
          "Reads the framebuffer the shader just drew. Returns a text summary "
          "(path, size, mean color, 8x8 grid) and the PNG itself.")}]]
  mcp::content
  capture_preview();

  [[= mcp::resource{.uri = js::str("shader-editor://uniforms"),
                    .mime_type = js::str("text/plain")}]][[= js::doc{
      .text = js::str(
          "Built-in uniforms the shader editor provides, and the vertex input "
          "the fullscreen quad uses.")}]] std::string
  uniform_catalog();

protected:
  std::string_view server_name() const override { return "shader-editor"; }
  std::string_view server_version() const override { return "0.1.0"; }
  std::string_view instructions() const override {
    return "Edit the live shader with set_shader, then capture_preview to see "
           "the frame. Use only the built-in uniforms from builtin_uniforms.";
  }

private:
  shader_editor_scene *m_scene;
  std::thread m_thread;
};
