#pragma once

#include <memory>
#include <string>
#include <vector>

class texture_2d;

// The LangGraph chat. It owns the Python process and the ImGui transcript,
// so the shader editor scene only draws it.
class shader_editor_chat {
public:
  shader_editor_chat() = default;
  ~shader_editor_chat();

  shader_editor_chat(const shader_editor_chat &) = delete;
  shader_editor_chat &operator=(const shader_editor_chat &) = delete;

  void draw();

private:
  struct entry {
    std::string kind;
    std::string text;
  };

  void start();
  void stop();
  void poll();
  void send();
  void handle_line(const std::string &line);

  std::vector<entry> m_log;
  char m_input[4096]{};
  int m_pid = -1;
  int m_in = -1;
  int m_out = -1;
  std::string m_buf;
  bool m_ready = false;
  bool m_scroll = false;
  std::string m_preview_path;
  std::unique_ptr<texture_2d> m_preview;
};
