#pragma once

#include "basic/shader.h"
#include "scene_base.h"
#include "tests/component/connection.h"
#include "tests/component/mesh_manager.h"
#include <array>
#include <atomic>
#include <string>
#include <thread>
#include <vector>

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
};
