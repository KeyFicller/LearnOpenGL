#pragma once

#include "basic/shader.h"
#include "scene_base.h"
#include "tests/component/connection.h"
#include "tests/component/mesh_manager.h"
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class reveal_chess_chat;
class reveal_chess_mcp;

enum piece_type : unsigned int {
  k_none = 0,
  k_king = 1,
  k_guard,
  k_bishop,
  k_rook,
  k_horse,
  k_cannon,
  k_soldier,

  k_cover_mask = 0x00100,
  k_red_mask = 0x00200,
  k_black_mask = 0x00400,
};

enum class game_result { ongoing, red_win, black_win };

class reveal_chess_scene : public test_scene_base {
public:
  reveal_chess_scene();
  ~reveal_chess_scene() override;

public:
  void shuffle_board();
  // Public view of the board as a diagram. Covered pieces are not identified.
  std::string board_json();
  // Seconds left on the current turn, published as the clock tool.
  std::string clock_json();
  // Squares the piece on (row, col) can reach this turn, published as the moves
  // tool. Refuses a square whose piece cannot move this turn, and refuses once
  // the per-turn query budget is spent. Mechanical legality only: a move that
  // leaves the mover's own king en prise is still listed.
  std::string moves_json(int row, int col);
  // Seconds since the AI started working on the current turn.
  double turn_elapsed() const;
  // Black's move, used by the AI. Rejects Red's turn and illegal moves.
  // note is the agent's one-line reason; it never affects the game, only
  // guards against a move logged without any explanation.
  std::string play_move(int from_row, int from_col, int to_row, int to_col,
                        std::string_view note);
  // Runs fn on the thread that renders this scene, and waits for it.
  std::string on_render_thread(std::function<std::string()> fn);

private:
  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

  void on_object_hovered(int _object_id) override;
  bool on_mouse_button(int _button, int _action, int _mods) override;
  std::vector<std::pair<int, int>> get_valid_moves();

  void update(float _delta_time) override;

  // When connected: whether it's my turn (Server=red first, Client=black first)
  bool is_my_turn() const;
  // Apply the opponent's move, update the board and turn
  void apply_remote_move(int fr, int fc, int tr, int tc);
  void poll_network();
  void send_move(int fr, int fc, int tr, int tc);
  void send_board_sync();
  void send_heartbeat();
  void disconnect_peer();

  void draw_board();
  void draw_pieces();
  void draw_text();
  void draw_valid_moves();
  void draw_last_move();

  bool commit_move(int fr, int fc, int tr, int tc, std::string *error);
  // Engine-only record of a move, appended to
  // .output/chess/<stamp>.truth.jsonl. A covered piece that is captured never
  // shows its type, so the public log cannot say what was taken. This can.
  void record_move(int fr, int fc, int tr, int tc, unsigned int mover,
                   unsigned int target, bool red_moved);
  void forget_history();
  std::string truth_path() const;
  void drain_mcp_tasks();
  void restart_turn_clock();
  void enforce_clock();

private:
  shader *m_board_shader = nullptr;
  shader *m_piece_shader = nullptr;
  shader *m_valid_move_shader = nullptr;
  shader *m_last_move_shader = nullptr;
  mesh_manager m_board_mesh_manager;
  mesh_manager m_piece_mesh_manager;
  mesh_manager m_valid_move_mesh_manager;
  mesh_manager m_last_move_mesh_manager;
  int m_board[10][9];
  std::pair<int, int> m_selected_piece = {-1, -1};
  std::pair<int, int> m_last_move_from = {-1, -1};
  std::pair<int, int> m_last_move_to = {-1, -1};
  std::pair<int, int> m_hovered_piece = {-1, -1};

  bool m_red_turn = true;
  game_result m_game_result = game_result::ongoing;

  server *m_server = nullptr;
  client *m_client = nullptr;
  enum class connect_mode {
    none,
    server,
    client,
  };
  connect_mode m_connect_mode = connect_mode::none;
  int m_port = 8888;
  std::string m_host = "127.0.0.1";

  std::atomic<bool> m_server_ready{false};
  std::atomic<bool> m_server_starting{false};
  std::thread m_server_thread;
  static constexpr size_t k_move_msg_size = 17; // 1 type + 4*4 bytes
  std::string m_recv_buf;
  bool m_board_sync_sent = false;
  bool m_board_sync_received = false; // client: need to receive board sync from
                                      // server before making a move
  bool m_cheat_reveal_all = false;

  double m_last_recv_time = 0;
  double m_last_heartbeat_sent_time = 0;

  bool m_red_ai = true;
  bool m_black_ai = true;
  bool m_ai_thinking = true;
  int m_red_illegal = 0;
  int m_black_illegal = 0;
  // Whether that side's last move left the other king attacked, i.e. whether
  // it gave check. A king may only be taken after a check the taker gave
  // itself, so this is what makes the capture legal.
  bool m_red_check_given = false;
  bool m_black_check_given = false;
  // Seconds an AI side gets for one move. The agent retries once without
  // thinking, so the engine only forfeits after two missed budgets.
  int m_move_seconds = 60;
  // moves calls spent this turn. Reset with the clock, so the cap is per turn.
  int m_move_queries = 0;
  std::chrono::steady_clock::time_point m_turn_started =
      std::chrono::steady_clock::now();
  bool m_was_thinking = false;
  int m_ply = 0;
  std::string m_log_stamp;
  std::unique_ptr<reveal_chess_chat> m_chat;
  std::unique_ptr<reveal_chess_mcp> m_mcp;
  std::mutex m_mcp_mu;
  struct mcp_task {
    std::uint64_t id;
    std::function<void()> run;
  };
  std::vector<mcp_task> m_mcp_tasks;
  std::atomic<std::uint64_t> m_mcp_seq{0};
};
