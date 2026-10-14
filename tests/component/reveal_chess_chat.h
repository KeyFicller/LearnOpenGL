#pragma once

#include <string>
#include <vector>

// Shows the chess AI's reasoning. There is no input: the process plays on
// its own and writes one JSON line per thought.
class reveal_chess_chat {
public:
  reveal_chess_chat() = default;
  ~reveal_chess_chat();

  reveal_chess_chat(const reveal_chess_chat &) = delete;
  reveal_chess_chat &operator=(const reveal_chess_chat &) = delete;

  bool running() const { return m_pid > 0; }
  // True only while a model request is in flight, not merely because it is
  // that side's turn.
  bool thinking() const { return m_thinking; }
  const std::string &thinking_side() const { return m_thinking_side; }
  void start(const std::string &red_provider, const std::string &red_model,
             const std::string &black_provider, const std::string &black_model,
             bool thinking);
  void stop();
  void draw();

private:
  struct entry {
    std::string kind;
    std::string text;
    std::string side;
  };

  void poll();
  void handle_line(const std::string &line);

  std::vector<entry> m_log;
  int m_pid = -1;
  int m_out = -1;
  std::string m_buf;
  std::string m_thinking_side;
  bool m_scroll = false;
  bool m_thinking = false;
};
