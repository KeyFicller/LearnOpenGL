#pragma once

// MCP server for the reveal-chess AI. Tools are ordinary members; the base
// class publishes them by reflection. Board access hops to the render thread.

#include "llm/reflection_mcp.h"

#include <thread>

class reveal_chess_scene;

inline constexpr int k_reveal_chess_mcp_port = 8766;

class reveal_chess_mcp : public mcp::Server<reveal_chess_mcp> {
public:
  explicit reveal_chess_mcp(reveal_chess_scene *scene);
  ~reveal_chess_mcp() override;

  void start();

  [[= mcp::tool{.read_only = true, .open_world = false}]][[= js::doc{
      .text = js::str(
          "The public position as a markdown table. 红· and 黑· are covered "
          "pieces with no hidden type. A face-up cell is 红 or 黑 plus the "
          "piece name, so 红马 and 黑马 differ. Row 0 is the top. Does not "
          "list legal moves.")}]]
  std::string
  board();

  [[= mcp::tool{.read_only = false,
                .destructive = false,
                .idempotent = false,
                .open_world = false}]][[= js::doc{
      .text = js::str(
          "Play one move for the side to move. Rows and columns are the "
          "diagram indexes, 0 through 9 and 0 through 8. An illegal move "
          "returns an error and does not change the board. Does not reveal "
          "a covered piece's type.")}]]
  [[= js::param_docs(js::str("Source row, 0 at Red's back rank through 9."),
                     js::str("Source column, 0 through 8, left to right."),
                     js::str("Destination row."),
                     js::str("Destination column."))]] std::string
  play(int from_row, int from_col, int to_row, int to_col);

protected:
  std::string_view server_name() const override { return "reveal-chess"; }
  std::string_view server_version() const override { return "0.1.0"; }
  std::string_view instructions() const override {
    return "Call board, then decide one move from the rules and play it.";
  }

private:
  reveal_chess_scene *m_scene;
  std::thread m_thread;
};
