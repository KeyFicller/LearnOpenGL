#include "reveal_chess_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>

#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_quad.h"

namespace {

// Default piece type at standard position (for unrevealed move rule).
piece_type default_type_at(int _r, int _c) {
  if (_r == 0 || _r == 9 - 0) {
    if (_c == 4)
      return piece_type::k_king;
    if (_c == 0 || _c == 8)
      return piece_type::k_rook;
    if (_c == 1 || _c == 7)
      return piece_type::k_horse;
    if (_c == 2 || _c == 6)
      return piece_type::k_bishop;
    if (_c == 3 || _c == 5)
      return piece_type::k_guard;
  }
  if ((_r == 2 || _r == 9 - 2) && (_c == 1 || _c == 7))
    return piece_type::k_cannon;

  if ((_r == 3 || _r == 9 - 3) &&
      (_c == 0 || _c == 2 || _c == 4 || _c == 6 || _c == 8))
    return piece_type::k_soldier;

  return piece_type::k_none;
}

static bool in_bounds(int _r, int _c) {
  return _r >= 0 && _r < 10 && _c >= 0 && _c < 9;
}

static bool is_friend(int _first, int _second) {
  if ((_first & piece_type::k_cover_mask) != 0 ||
      (_second & piece_type::k_cover_mask) != 0)
    return false;

  if ((_first & piece_type::k_red_mask) != 0 &&
      (_second & piece_type::k_red_mask) != 0)
    return true;
  if ((_first & piece_type::k_black_mask) != 0 &&
      (_second & piece_type::k_black_mask) != 0)
    return true;
  return false;
}

// When is_covered: use default position rules (king/guard in palace, bishop not
// cross river). When revealed: no position limit (e.g. 仕 on opponent side can
// move one diag anywhere).
std::vector<std::pair<int, int>>
get_valid_moves_for_piece(const int board[10][9], piece_type _piece_type,
                          int _r, int _c) {
  std::vector<std::pair<int, int>> valid_moves;
  auto add_if_ok = [&](int _r, int _c) {
    if (!in_bounds(_r, _c))
      return;
    if (board[_r][_c] == 0 || !is_friend(board[_r][_c], _piece_type))
      valid_moves.push_back({_r, _c});
  };

  bool is_self_covered = (_piece_type & piece_type::k_cover_mask) != 0;
  int base_piece_type =
      is_self_covered ? default_type_at(_r, _c) : (_piece_type & 0xFF);
  switch (base_piece_type) {
  case piece_type::k_king: {
    static const int king_dr[] = {1, -1, 0, 0}, king_dc[] = {0, 0, 1, -1};
    for (int i = 0; i < 4; i++) {
      int nr = _r + king_dr[i], nc = _c + king_dc[i];
      if (is_self_covered) {
        if (in_bounds(nr, nc))
          add_if_ok(nr, nc);
      } else {
        const bool is_red = (_piece_type & piece_type::k_red_mask) != 0;
        if (is_red && nr >= 0 && nr <= 2 && nc >= 3 && nc <= 5)
          add_if_ok(nr, nc);
        if (!is_red && nr >= 7 && nr <= 9 && nc >= 3 && nc <= 5)
          add_if_ok(nr, nc);
      }
    }
    break;
  }
  case piece_type::k_guard: {
    static const int guard_dr[] = {1, 1, -1, -1}, guard_dc[] = {1, -1, 1, -1};
    for (int i = 0; i < 4; i++) {
      int nr = _r + guard_dr[i], nc = _c + guard_dc[i];
      if (is_self_covered) {
        if (in_bounds(nr, nc))
          add_if_ok(nr, nc);
      } else {
        const bool is_red = (_piece_type & piece_type::k_red_mask) != 0;
        if (is_red && nr >= 0 && nr <= 2 && nc >= 3 && nc <= 5)
          add_if_ok(nr, nc);
        if (!is_red && nr >= 7 && nr <= 9 && nc >= 3 && nc <= 5)
          add_if_ok(nr, nc);
      }
    }
    break;
  }
  case piece_type::k_bishop: {
    static const int bishop_dr[] = {2, 2, -2, -2}, bishop_dc[] = {2, -2, 2, -2};
    for (int i = 0; i < 4; i++) {
      int nr = _r + bishop_dr[i], nc = _c + bishop_dc[i];
      int leg_r = _r + bishop_dr[i] / 2, leg_c = _c + bishop_dc[i] / 2;
      if (!in_bounds(nr, nc))
        continue;
      if (board[leg_r][leg_c] != 0)
        continue;

      if (is_self_covered) {
        add_if_ok(nr, nc);
      } else {
        const bool is_red = (_piece_type & piece_type::k_red_mask) != 0;
        if (is_red && nr <= 4)
          add_if_ok(nr, nc);
        if (!is_red && nr >= 5)
          add_if_ok(nr, nc);
      }
    }
    break;
  }
  case piece_type::k_rook: {
    static const int rook_dr[] = {1, -1, 0, 0}, rook_dc[] = {0, 0, 1, -1};
    for (int d = 0; d < 4; d++) {
      for (int step = 1; step < 10; step++) {
        int nr = _r + step * rook_dr[d], nc = _c + step * rook_dc[d];
        if (!in_bounds(nr, nc))
          break;
        add_if_ok(nr, nc);
      }
    }
    break;
  }
  case piece_type::k_horse: {
    static const int horse_dr[] = {2, 2, -2, -2, 1, 1, -1, -1};
    static const int horse_dc[] = {1, -1, 1, -1, 2, -2, 2, -2};
    for (int i = 0; i < 8; i++) {
      int dr = horse_dr[i], dc = horse_dc[i];
      int leg_r = _r + dr / 2, leg_c = _c + dc / 2;
      if (!in_bounds(leg_r, leg_c) || board[leg_r][leg_c] != 0)
        continue;
      int nr = _r + dr, nc = _c + dc;
      add_if_ok(nr, nc);
    }
    break;
  }
  case piece_type::k_cannon: {
    static const int cannon_dr[] = {1, -1, 0, 0}, cannon_dc[] = {0, 0, 1, -1};
    for (int d = 0; d < 4; d++) {
      int count = 0;
      for (int step = 1; step < 10; step++) {
        int nr = _r + step * cannon_dr[d], nc = _c + step * cannon_dc[d];
        if (!in_bounds(nr, nc))
          break;
        if (board[nr][nc] == 0) {
          if (count == 0)
            valid_moves.push_back({nr, nc});
        } else {
          count++;
          if (count == 1)
            continue;
          if (count == 2 && !is_friend(board[nr][nc], _piece_type)) {
            valid_moves.push_back({nr, nc});
          }
          break;
        }
      }
    }
    break;
  }
  case piece_type::k_soldier: {
    const bool is_red = (_piece_type & piece_type::k_red_mask) != 0;
    int forward_dr = 1;
    if (is_self_covered) {
      forward_dr = _r < 5 ? 1 : -1;
    } else {
      forward_dr = is_red ? 1 : -1;
    }

    add_if_ok(_r + forward_dr, _c);

    if (!is_self_covered) {
      if (is_red && _r >= 5) {
        add_if_ok(_r, _c + 1);
        add_if_ok(_r, _c - 1);
      } else if (!is_red && _r <= 4) {
        add_if_ok(_r, _c + 1);
        add_if_ok(_r, _c - 1);
      }
    }

    break;
  }
  default:
    break;
  }
  return valid_moves;
}

bool piece_index_is_valid(const std::pair<int, int> &_index) {
  return _index.first != -1 && _index.second != -1;
}

void invalidate_piece_index(std::pair<int, int> &_index) { _index = {-1, -1}; }

} // namespace

reveal_chess_scene::reveal_chess_scene() : test_scene_base("Reveal Chess") {
  std::memset(m_board, 0, sizeof(m_board));
}

reveal_chess_scene::~reveal_chess_scene() {
  if (m_board_shader)
    delete m_board_shader;
  if (m_piece_shader)
    delete m_piece_shader;
  if (m_valid_move_shader)
    delete m_valid_move_shader;
}

void reveal_chess_scene::init(GLFWwindow *_window) {
  test_scene_base::init(_window);

  m_board_shader =
      new shader("shaders/reveal_chess_test/chess_board/vertex.shader",
                 "shaders/reveal_chess_test/chess_board/fragment.shader",
                 "shaders/reveal_chess_test/chess_board/geometry.shader");

  glm::vec3 center(0.0f, 0.0f, 0.0f);
  mesh_data board_mesh_data(&center[0], sizeof(center), 1,
                            {{3, GL_FLOAT, GL_FALSE}});
  m_board_mesh_manager.setup_mesh(board_mesh_data);

  m_piece_shader =
      new shader("shaders/reveal_chess_test/chess_piece/vertex.shader",
                 "shaders/reveal_chess_test/chess_piece/fragment.shader",
                 "shaders/reveal_chess_test/chess_piece/geometry.shader");

  m_valid_move_shader = new shader(
      "shaders/reveal_chess_test/chess_hint/valid_move_vertex.shader",
      "shaders/reveal_chess_test/chess_hint/valid_move_fragment.shader",
      "shaders/reveal_chess_test/chess_hint/valid_move_geometry.shader");
  shuffle_board();
}

void reveal_chess_scene::shuffle_board() {
  std::memset(m_board, 0, sizeof(m_board));
  // Put king
  m_board[0][4] = piece_type::k_king | piece_type::k_red_mask;
  m_board[9][4] = piece_type::k_king | piece_type::k_black_mask;

  // Shuffle the remaining pieces
  static std::vector<std::pair<int, int>> positions = {
      // Red pieces
      {0, 0},
      {0, 1},
      {0, 2},
      {0, 3},
      {0, 5},
      {0, 6},
      {0, 7},
      {0, 8},
      // Black pieces
      {9, 0},
      {9, 1},
      {9, 2},
      {9, 3},
      {9, 5},
      {9, 6},
      {9, 7},
      {9, 8},
      // Read Cannons
      {2, 1},
      {2, 7},
      // Black Cannons
      {7, 1},
      {7, 7},
      // Read Soldiers
      {3, 0},
      {3, 2},
      {3, 4},
      {3, 6},
      {3, 8},
      // Black Soldiers
      {6, 0},
      {6, 2},
      {6, 4},
      {6, 6},
      {6, 8},
  };

  std::vector<int> pieces;
  // Put red pieces
  for (int i = 0; i < 2; i++) {
    int mask = i == 0 ? piece_type::k_red_mask : piece_type::k_black_mask;
    mask |= piece_type::k_cover_mask;
    for (int i = 0; i < 2; i++) {
      pieces.push_back(piece_type::k_guard | mask);
      pieces.push_back(piece_type::k_bishop | mask);
      pieces.push_back(piece_type::k_horse | mask);
      pieces.push_back(piece_type::k_rook | mask);
      pieces.push_back(piece_type::k_cannon | mask);
    }
    for (int i = 0; i < 5; i++) {
      pieces.push_back(piece_type::k_soldier | mask);
    }
  }
  std::shuffle(pieces.begin(), pieces.end(), std::default_random_engine{});
  for (int i = 0; i < pieces.size(); i++) {
    int r = positions[i].first;
    int c = positions[i].second;
    m_board[r][c] = pieces[i];
  }
}

void reveal_chess_scene::draw_board() {
  m_board_shader->use();
  m_board_mesh_manager.bind();
  glDrawArrays(GL_POINTS, 0, m_board_mesh_manager.get_index_count());
}

void reveal_chess_scene::draw_pieces() {
  // Update mesh data
  std::vector<std::pair<int, int>> piece_positions;
  for (int r = 0; r < 10; r++) {
    for (int c = 0; c < 9; c++) {
      if (m_board[r][c]) {
        piece_positions.push_back({r, c});
      }
    }
  }

  std::vector<int> piece_data;
  for (auto &[r, c] : piece_positions) {
    piece_data.push_back(r);
    piece_data.push_back(c);
    piece_data.push_back(m_board[r][c]);
    piece_data.push_back(r * 10 + c);
  }
  mesh_data piece_mesh_data(piece_data.data(), piece_data.size() * sizeof(int),
                            piece_positions.size(),
                            {{1, GL_INT, GL_FALSE},
                             {1, GL_INT, GL_FALSE},
                             {1, GL_INT, GL_FALSE},
                             {1, GL_INT, GL_FALSE}});
  m_piece_mesh_manager.setup_mesh(piece_mesh_data);

  m_piece_shader->use();
  if (m_hovered_piece.first != -1 && m_hovered_piece.second != -1) {
    m_piece_shader->set_uniform("uHoveredPieceId", m_hovered_piece.first * 10 +
                                                       m_hovered_piece.second);
  } else {
    m_piece_shader->set_uniform("uHoveredPieceId", -1);
  }
  if (m_selected_piece.first != -1 && m_selected_piece.second != -1) {
    m_piece_shader->set_uniform("uSelectedPieceId",
                                m_selected_piece.first * 10 +
                                    m_selected_piece.second);
  } else {
    m_piece_shader->set_uniform("uSelectedPieceId", -1);
  }
  m_piece_mesh_manager.bind();
  glDrawArrays(GL_POINTS, 0, m_piece_mesh_manager.get_index_count());
}

void reveal_chess_scene::draw_valid_moves() {
  if (m_selected_piece.first == -1 || m_selected_piece.second == -1)
    return;

  std::vector<std::pair<int, int>> valid_moves = get_valid_moves();
  std::vector<int> piece_data;
  for (auto &[r, c] : valid_moves) {
    piece_data.push_back(r);
    piece_data.push_back(c);
  }
  mesh_data valid_move_mesh_data(
      piece_data.data(), piece_data.size() * sizeof(int), valid_moves.size(),
      {{1, GL_INT, GL_FALSE}, {1, GL_INT, GL_FALSE}});
  m_valid_move_mesh_manager.setup_mesh(valid_move_mesh_data);

  m_valid_move_shader->use();
  m_valid_move_mesh_manager.bind();
  glDrawArrays(GL_POINTS, 0, m_valid_move_mesh_manager.get_index_count());
}

void reveal_chess_scene::render() {
  if (!m_board_shader || !m_piece_shader || !m_valid_move_shader)
    return;
  draw_board();
  draw_pieces();
  draw_valid_moves();
}

void reveal_chess_scene::render_ui() {
  ImGui::Text("Reveal Chess");
  ImGui::Separator();
  ImGui::Text("Hovered piece: %d, %d", m_hovered_piece.first,
              m_hovered_piece.second);
}

void reveal_chess_scene::on_object_hovered(int _object_id) {
  int r = _object_id / 10;
  int c = _object_id % 10;
  if (_object_id == -1) {
    m_hovered_piece = {-1, -1};
    return;
  }
  m_hovered_piece = {r, c};
}

bool reveal_chess_scene::on_mouse_button(int _button, int _action, int _mods) {
  if (_button == GLFW_MOUSE_BUTTON_LEFT && _action == GLFW_PRESS) {
    if (piece_index_is_valid(m_selected_piece)) {
      if (!piece_index_is_valid(m_hovered_piece)) {
        invalidate_piece_index(m_selected_piece);
        return true;
      } else {
        if (m_selected_piece == m_hovered_piece) {
          invalidate_piece_index(m_selected_piece);
          return true;
        } else {
          auto valid_moves = get_valid_moves();
          auto iter = std::find(valid_moves.begin(), valid_moves.end(),
                                m_hovered_piece);
          if (iter == valid_moves.end()) {
            m_selected_piece = m_hovered_piece;
          } else {
            auto sel_piece_type =
                m_board[m_selected_piece.first][m_selected_piece.second];

            m_board[m_hovered_piece.first][m_hovered_piece.second] =
                sel_piece_type & (~piece_type::k_cover_mask);

            m_board[m_selected_piece.first][m_selected_piece.second] = 0;

            invalidate_piece_index(m_selected_piece);
          }
        }
      }
    } else {
      m_selected_piece = m_hovered_piece;
    }
    return true;
  }
  return false;
}

std::vector<std::pair<int, int>> reveal_chess_scene::get_valid_moves() {
  if (m_selected_piece.first == -1 || m_selected_piece.second == -1)
    return {};

  int r = m_selected_piece.first, c = m_selected_piece.second;
  unsigned int cell = m_board[r][c];

  return get_valid_moves_for_piece(m_board, static_cast<piece_type>(cell), r,
                                   c);
}