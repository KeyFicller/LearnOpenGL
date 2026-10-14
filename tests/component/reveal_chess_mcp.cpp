#include "tests/component/reveal_chess_mcp.h"

#include "tests/scenes/reveal_chess_scene.h"

reveal_chess_mcp::reveal_chess_mcp(reveal_chess_scene *scene)
    : m_scene(scene) {}

reveal_chess_mcp::~reveal_chess_mcp() {
  stop_http();
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void reveal_chess_mcp::start() {
  if (m_thread.joinable()) {
    return;
  }
  m_thread = std::thread([this] { serve_http(k_reveal_chess_mcp_port); });
}

std::string reveal_chess_mcp::board() {
  return m_scene->on_render_thread([this] { return m_scene->board_json(); });
}

std::string reveal_chess_mcp::play(int from_row, int from_col, int to_row,
                                   int to_col) {
  return m_scene->on_render_thread([this, from_row, from_col, to_row, to_col] {
    return m_scene->play_move(from_row, from_col, to_row, to_col);
  });
}
