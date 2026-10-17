#pragma once

// MCP server for the reveal-chess AI. Tools are ordinary members; the base
// class publishes them by reflection. Board access hops to the render thread.

#include "llm/reflection_mcp.h"

#include <thread>

class reveal_chess_scene;

inline constexpr int k_reveal_chess_mcp_port = 8766;
// How many times one turn may ask which squares a piece can reach. The cap
// lives here because it is part of the tool's contract: the description states
// it, the engine enforces it.
inline constexpr int k_reveal_chess_move_queries = 4;

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
          "list legal moves. check lists the sides whose king is under attack "
          "right now; may_take_king says whether the side to move has the right "
          "to take the enemy king.")}]]
  std::string
  board();

  [[= mcp::tool{.read_only = true, .open_world = false}]][[= js::doc{
      .text = js::str(
          "Time left on the current turn: limit, elapsed and seconds_left. "
          "seconds_left turns negative once the budget is spent, so call it "
          "before a long think rather than after.")}]]
  std::string
  clock();

  [[= mcp::tool{.read_only = true, .open_world = false}]][[= js::doc{
      .text = js::str(
          "The squares one piece can reach this turn. Ask about a single "
          "piece, the way a player studies one piece instead of generating "
          "every move. The list is the engine's own, so it is mechanical "
          "legality only: a square that would leave your own king able to be "
          "captured is still on it and is still a losing move. The one rule it "
          "does apply is the flying general, so a move that would leave the "
          "two kings facing on an open line is not listed. The enemy king is "
          "listed like any other square, but play only accepts taking it when "
          "the board's may_take_king is true. A square that "
          "is empty, off the board, or not yours this turn is refused with no "
          "squares, because the piece cannot move this turn. At most 4 calls "
          "per turn, refused calls included; the reply says how many are "
          "left.")}]]
  [[= js::param_docs(js::str("Row of the piece, 0 at Red's back rank through 9."),
                     js::str("Column of the piece, 0 through 8, left to right."))]]
  std::string
  moves(int row, int col);

  [[= mcp::tool{.read_only = false,
                .destructive = false,
                .idempotent = false,
                .open_world = false}]][[= js::doc{
      .text = js::str(
          "Play one move for the side to move. Rows and columns are the "
          "diagram indexes, 0 through 9 and 0 through 8. An illegal move "
          "returns an error and does not change the board. Does not reveal "
          "a covered piece's type. note is required: one or two Chinese "
          "sentences saying what you are doing and why, for the match log. "
          "It is not shown to the opponent. A move that checks the enemy king "
          "is announced by the board's check field, so the opponent is always "
          "told; you do not need to say 将军 anywhere. Taking the enemy king "
          "is only accepted after a check you gave on your own previous move, "
          "which is what may_take_king reports.")}]]
  [[= js::param_docs(js::str("Source row, 0 at Red's back rank through 9."),
                     js::str("Source column, 0 through 8, left to right."),
                     js::str("Destination row."),
                     js::str("Destination column."),
                     js::str("One or two Chinese sentences on why this move. "
                             "Conclusions only; no worked-through lines."))]]
  std::string
  play(int from_row, int from_col, int to_row, int to_col, std::string note);

protected:
  std::string_view server_name() const override { return "reveal-chess"; }
  std::string_view server_version() const override { return "0.1.0"; }
  std::string_view instructions() const override {
    return "Call board, then decide one move from the rules and play it. clock "
           "reports the time left on this turn. moves answers one piece at a "
           "time and is capped per turn.";
  }

private:
  reveal_chess_scene *m_scene;
  std::thread m_thread;
};
