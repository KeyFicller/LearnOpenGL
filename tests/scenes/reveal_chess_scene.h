#pragma once

#include "basic/shader.h"
#include "scene_base.h"
#include "tests/component/mesh_manager.h"
#include <array>
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
  // For debug
  std::pair<int, int> m_hovered_piece = {-1, 1};
};
