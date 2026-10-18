#include "reveal_chess_scene.h"

#include "tests/component/reveal_chess_chat.h"
#include "tests/component/reveal_chess_mcp.h"

#include "glad/gl.h"
#include "glm/fwd.hpp"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <future>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>

#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_quad.h"
#include "tests/component/text_renderer.h"
#include "tests/component/interaction_utils.h"

#include <nlohmann/json.hpp>

namespace {
enum class msg_type : uint8_t {
  k_msg_move = 0,
  k_msg_board_sync = 1,
  k_msg_heartbeat = 2,
};
constexpr size_t k_move_msg_size = 17; // 1 type + 4 * int32
constexpr size_t k_board_sync_size = 1 + 10 * 9 * sizeof(int32_t); // 361
constexpr double k_heartbeat_interval = 2.0;
constexpr double k_heartbeat_timeout = 6.0;
// Absorbs the frame or two between starting the clock and the agent's own
// timer, so a retry that lands just inside its window is not rejected.
constexpr double k_move_grace = 3.0;

// Defined further down, where the diagram vocabulary lives. Declared here
// because the move helpers above them need to say which side a square is.
bool is_piece_of_current_side(bool _red_turn, unsigned int _cell, int _r,
                              int _c);

static void pack_move_msg(char *buf, int fr, int fc, int tr, int tc) {
  buf[0] = static_cast<char>(msg_type::k_msg_move);
  int32_t *p = reinterpret_cast<int32_t *>(buf + 1);
  p[0] = htonl(static_cast<int32_t>(fr));
  p[1] = htonl(static_cast<int32_t>(fc));
  p[2] = htonl(static_cast<int32_t>(tr));
  p[3] = htonl(static_cast<int32_t>(tc));
}

static bool unpack_move_msg(const char *buf, int *fr, int *fc, int *tr,
                            int *tc) {
  if (buf[0] != static_cast<char>(msg_type::k_msg_move))
    return false;
  const int32_t *p = reinterpret_cast<const int32_t *>(buf + 1);
  *fr = static_cast<int>(ntohl(p[0]));
  *fc = static_cast<int>(ntohl(p[1]));
  *tr = static_cast<int>(ntohl(p[2]));
  *tc = static_cast<int>(ntohl(p[3]));
  return true;
}

static void pack_board_sync(char *buf, const int board[10][9]) {
  buf[0] = static_cast<char>(msg_type::k_msg_board_sync);
  int32_t *p = reinterpret_cast<int32_t *>(buf + 1);
  for (int r = 0; r < 10; r++)
    for (int c = 0; c < 9; c++)
      *p++ = htonl(static_cast<int32_t>(board[r][c]));
}

static bool unpack_board_sync(const char *buf, int board[10][9]) {
  if (buf[0] != static_cast<char>(msg_type::k_msg_board_sync))
    return false;
  const int32_t *p = reinterpret_cast<const int32_t *>(buf + 1);
  for (int r = 0; r < 10; r++)
    for (int c = 0; c < 9; c++)
      board[r][c] = static_cast<int>(ntohl(*p++));
  return true;
}
} // namespace

namespace {

static const std::map<piece_type, std::string> piece_text_red = {
    {piece_type::k_king, "将"},    {piece_type::k_guard, "仕"},
    {piece_type::k_bishop, "相"},  {piece_type::k_rook, "车"},
    {piece_type::k_horse, "马"},   {piece_type::k_cannon, "炮"},
    {piece_type::k_soldier, "兵"},
};
static const std::map<piece_type, std::string> piece_text_black = {
    {piece_type::k_king, "帅"},    {piece_type::k_guard, "士"},
    {piece_type::k_bishop, "象"},  {piece_type::k_rook, "车"},
    {piece_type::k_horse, "马"},   {piece_type::k_cannon, "炮"},
    {piece_type::k_soldier, "卒"},
};

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

const char *piece_name(int base) {
  switch (base & 0xFF) {
  case piece_type::k_king:
    return "king";
  case piece_type::k_guard:
    return "guard";
  case piece_type::k_bishop:
    return "bishop";
  case piece_type::k_rook:
    return "rook";
  case piece_type::k_horse:
    return "horse";
  case piece_type::k_cannon:
    return "cannon";
  case piece_type::k_soldier:
    return "soldier";
  default:
    return "none";
  }
}

const char *result_name(game_result result) {
  if (result == game_result::red_win)
    return "red_win";
  if (result == game_result::black_win)
    return "black_win";
  return "ongoing";
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

// Xiangqi's flying-general rule: the two kings may never stand on the same
// open line. Occupancy is what blocks, so a covered piece blocks as well.
bool kings_facing(const int _board[10][9]) {
  int red_row = -1, red_col = -1, black_row = -1, black_col = -1;
  for (int r = 0; r < 10; ++r) {
    for (int c = 0; c < 9; ++c) {
      const unsigned int cell = static_cast<unsigned int>(_board[r][c]);
      if (cell == 0 || (cell & piece_type::k_cover_mask) != 0 ||
          (cell & 0xFF) != piece_type::k_king)
        continue;
      if ((cell & piece_type::k_red_mask) != 0) {
        red_row = r;
        red_col = c;
      } else {
        black_row = r;
        black_col = c;
      }
    }
  }
  if (red_row < 0 || black_row < 0)
    return false;
  if (red_row == black_row) {
    for (int c = std::min(red_col, black_col) + 1;
         c < std::max(red_col, black_col); ++c)
      if (_board[red_row][c] != 0)
        return false;
    return true;
  }
  if (red_col != black_col)
    return false;
  for (int r = std::min(red_row, black_row) + 1;
       r < std::max(red_row, black_row); ++r)
    if (_board[r][red_col] != 0)
      return false;
  return true;
}

// Whether playing this move would leave the kings facing. Only occupancy
// changes, so whether the mover is revealed on the way does not matter.
bool leaves_kings_facing(const int _board[10][9], int _fr, int _fc, int _tr,
                         int _tc) {
  int after[10][9];
  std::memcpy(after, _board, sizeof(after));
  after[_tr][_tc] = after[_fr][_fc];
  after[_fr][_fc] = 0;
  return kings_facing(after);
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
        if (!in_bounds(nr, nc))
          continue;
        if (_r == 0 && nr >= 0 && nr <= 2 && nc >= 3 && nc <= 5)
          add_if_ok(nr, nc);
        if (_r == 9 && nr >= 7 && nr <= 9 && nc >= 3 && nc <= 5)
          add_if_ok(nr, nc);
      } else {
        // const bool is_red = (_piece_type & piece_type::k_red_mask) != 0;
        // if (is_red && nr >= 0 && nr <= 2 && nc >= 3 && nc <= 5)
        //   add_if_ok(nr, nc);
        // if (!is_red && nr >= 7 && nr <= 9 && nc >= 3 && nc <= 5)
        //   add_if_ok(nr, nc);

        // Any position should be valid for revealed guard.
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
        // const bool is_red = (_piece_type & piece_type::k_red_mask) != 0;
        // if (is_red && nr <= 4)
        //   add_if_ok(nr, nc);
        // if (!is_red && nr >= 5)
        //   add_if_ok(nr, nc);

        // Any position should be valid for revealed bishop.
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
        if (board[nr][nc] != 0)
          break;
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

// What the board will actually accept: piece geometry plus the flying-general
// rule. Used wherever moves are shown to a player. commit_move checks the two
// separately, so it can say which one failed.
std::vector<std::pair<int, int>>
legal_moves_for_piece(const int _board[10][9], piece_type _piece_type, int _r,
                      int _c) {
  auto moves = get_valid_moves_for_piece(_board, _piece_type, _r, _c);
  std::erase_if(moves, [&](const std::pair<int, int> &_to) {
    return leaves_kings_facing(_board, _r, _c, _to.first, _to.second);
  });
  return moves;
}

// A face-up king of one colour. Covered pieces are never a king, and a king is
// always face up, so there is at most one square per colour.
std::pair<int, int> king_square(const int _board[10][9], bool _red) {
  for (int r = 0; r < 10; ++r) {
    for (int c = 0; c < 9; ++c) {
      const unsigned int cell = static_cast<unsigned int>(_board[r][c]);
      if (cell == 0 || (cell & piece_type::k_cover_mask) != 0 ||
          (cell & 0xFF) != piece_type::k_king)
        continue;
      if (((cell & piece_type::k_red_mask) != 0) == _red)
        return {r, c};
    }
  }
  return {-1, -1};
}

// Whether the side `_by_red` has a legal move onto the other king. That is a
// check: the other side must answer it on its turn.
bool side_checks(const int _board[10][9], bool _by_red) {
  const auto king = king_square(_board, !_by_red);
  if (king.first < 0)
    return false;
  for (int r = 0; r < 10; ++r) {
    for (int c = 0; c < 9; ++c) {
      const unsigned int cell = static_cast<unsigned int>(_board[r][c]);
      if (cell == 0 || !is_piece_of_current_side(_by_red, cell, r, c))
        continue;
      const auto moves =
          legal_moves_for_piece(_board, static_cast<piece_type>(cell), r, c);
      if (std::find(moves.begin(), moves.end(), king) != moves.end())
        return true;
    }
  }
  return false;
}

bool piece_index_is_valid(const std::pair<int, int> &_index) {
  return _index.first != -1 && _index.second != -1;
}

void invalidate_piece_index(std::pair<int, int> &_index) { _index = {-1, -1}; }

} // namespace

reveal_chess_scene::reveal_chess_scene() : test_scene_base("Reveal Chess") {
  std::memset(m_board, 0, sizeof(m_board));
  m_chat = std::make_unique<reveal_chess_chat>();
}

reveal_chess_scene::~reveal_chess_scene() {
  m_chat.reset();
  m_mcp.reset();
  if (m_server_thread.joinable())
    m_server_thread.join();
  delete m_server;
  m_server = nullptr;
  delete m_client;
  m_client = nullptr;
  if (m_board_shader)
    delete m_board_shader;
  if (m_piece_shader)
    delete m_piece_shader;
  if (m_valid_move_shader)
    delete m_valid_move_shader;
  if (m_last_move_shader)
    delete m_last_move_shader;
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
  m_last_move_shader = new shader(
      "shaders/reveal_chess_test/chess_hint/last_move_vertex.shader",
      "shaders/reveal_chess_test/chess_hint/last_move_fragment.shader",
      "shaders/reveal_chess_test/chess_hint/last_move_geometry.shader");
  shuffle_board();
  m_mcp = std::make_unique<reveal_chess_mcp>(this);
  m_mcp->start();
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
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(pieces.begin(), pieces.end(), g);
  for (size_t i = 0; i < pieces.size(); i++) {
    int r = positions[i].first;
    int c = positions[i].second;
    m_board[r][c] = pieces[i];
  }

  m_red_turn = true;
  m_game_result = game_result::ongoing;
  m_red_illegal = 0;
  m_black_illegal = 0;
  m_red_check_given = false;
  m_black_check_given = false;
  restart_turn_clock();
  forget_history();
  invalidate_piece_index(m_last_move_from);
  invalidate_piece_index(m_last_move_to);
  invalidate_piece_index(m_selected_piece);
  invalidate_piece_index(m_hovered_piece);
}

bool reveal_chess_scene::is_my_turn() const {
  if (m_connect_mode == connect_mode::none)
    return true;
  if (m_connect_mode == connect_mode::server)
    return m_red_turn;
  return m_board_sync_received && !m_red_turn;
}

void reveal_chess_scene::apply_remote_move(int fr, int fc, int tr, int tc) {
  if (!in_bounds(fr, fc) || !in_bounds(tr, tc))
    return;
  unsigned int piece = m_board[fr][fc];
  unsigned int captured = m_board[tr][tc];
  const bool moved_by_red = m_red_turn;
  record_move(fr, fc, tr, tc, piece, captured, moved_by_red);
  m_board[tr][tc] = piece & (~piece_type::k_cover_mask);
  m_board[fr][fc] = 0;
  m_last_move_from = {fr, fc};
  m_last_move_to = {tr, tc};
  invalidate_piece_index(m_selected_piece);

  if (captured != 0) {
    piece_type pt = static_cast<piece_type>(captured & 0xFF);
    if (pt == piece_type::k_king)
      m_game_result =
          moved_by_red ? game_result::red_win : game_result::black_win;
  }
  // Same announcement as a local move, so both sides agree on who may take a
  // king.
  if (moved_by_red)
    m_red_check_given = side_checks(m_board, true);
  else
    m_black_check_given = side_checks(m_board, false);
  if (m_game_result == game_result::ongoing)
    m_red_turn = !m_red_turn;
  restart_turn_clock();
}

void reveal_chess_scene::send_move(int fr, int fc, int tr, int tc) {
  char buf[k_move_msg_size];
  pack_move_msg(buf, fr, fc, tr, tc);
  if (m_server)
    m_server->send(buf, sizeof(buf));
  else if (m_client && m_client->is_connected())
    m_client->send(buf, sizeof(buf));
}

void reveal_chess_scene::send_board_sync() {
  if (!m_server || m_board_sync_sent)
    return;
  char buf[k_board_sync_size];
  pack_board_sync(buf, m_board);
  if (m_server->send(buf, sizeof(buf)) == static_cast<std::ptrdiff_t>(sizeof(buf)))
    m_board_sync_sent = true;
}

void reveal_chess_scene::send_heartbeat() {
  const char buf = static_cast<char>(msg_type::k_msg_heartbeat);
  if (m_server)
    m_server->send(&buf, 1);
  else if (m_client && m_client->is_connected())
    m_client->send(&buf, 1);
}

void reveal_chess_scene::disconnect_peer() {
  if (m_server) {
    delete m_server;
    m_server = nullptr;
  }
  if (m_client) {
    delete m_client;
    m_client = nullptr;
  }
  m_connect_mode = connect_mode::none;
  m_board_sync_sent = false;
  m_board_sync_received = false;
  m_recv_buf.clear();
}

void reveal_chess_scene::poll_network() {
  if (m_connect_mode == connect_mode::none)
    return;
  char tmp[512];
  std::ptrdiff_t n = -1;
  if (m_server)
    n = m_server->recv(tmp, sizeof(tmp));
  else if (m_client && m_client->is_connected())
    n = m_client->recv(tmp, sizeof(tmp));
  if (n < 0)
    return;
  if (n == 0) {
    disconnect_peer();
    return;
  }
  m_recv_buf.append(tmp, static_cast<size_t>(n));
  m_last_recv_time = glfwGetTime();

  while (m_recv_buf.size() >= 1) {
    uint8_t type = static_cast<uint8_t>(m_recv_buf[0]);
    if (type == static_cast<uint8_t>(msg_type::k_msg_move) &&
        m_recv_buf.size() >= k_move_msg_size) {
      int fr, fc, tr, tc;
      if (unpack_move_msg(m_recv_buf.data(), &fr, &fc, &tr, &tc))
        apply_remote_move(fr, fc, tr, tc);
      m_recv_buf.erase(0, k_move_msg_size);
    } else if (type == static_cast<uint8_t>(msg_type::k_msg_board_sync) &&
               m_recv_buf.size() >= k_board_sync_size) {
      if (unpack_board_sync(m_recv_buf.data(), m_board)) {
        m_red_turn = true;
        m_game_result = game_result::ongoing;
        m_board_sync_received = true;
        invalidate_piece_index(m_last_move_from);
        invalidate_piece_index(m_last_move_to);
        invalidate_piece_index(m_selected_piece);
      }
      m_recv_buf.erase(0, k_board_sync_size);
    } else if (type == static_cast<uint8_t>(msg_type::k_msg_heartbeat)) {
      m_recv_buf.erase(0, 1);
    } else {
      break;
    }
  }
}

void reveal_chess_scene::update(float _delta_time) {
  (void)_delta_time;
  if (m_server_ready) {
    m_server_ready = false;
    m_connect_mode = connect_mode::server;
    m_server_starting = false;
    if (m_server) {
      m_server->set_non_blocking(true);
      m_last_recv_time = glfwGetTime();
      m_last_heartbeat_sent_time = m_last_recv_time;
      shuffle_board();
      send_board_sync();
    }
  }
  poll_network();
  // The budget covers the agent working on the turn, not the taunts and the
  // polling that run before it picks the turn up. The chat parses the agent's
  // "thinking" line, so this is the moment its own timer starts too.
  if (m_chat) {
    const bool thinking = m_chat->thinking();
    if (thinking && !m_was_thinking)
      restart_turn_clock();
    m_was_thinking = thinking;
  }
  enforce_clock();

  if (m_connect_mode != connect_mode::none) {
    const double now = glfwGetTime();
    if (now - m_last_recv_time > k_heartbeat_timeout) {
      disconnect_peer();
      return;
    }
    if (now - m_last_heartbeat_sent_time >= k_heartbeat_interval) {
      send_heartbeat();
      m_last_heartbeat_sent_time = now;
    }
  }
}

namespace {

// Board coordinates are a square grid in NDC, so a wide viewport would stretch
// the board and draw the pieces as ovals. Scaling x by height/width keeps a
// board-space unit the same number of pixels on both axes.
float viewport_aspect_x() {
  GLint vp[4];
  glGetIntegerv(GL_VIEWPORT, vp);
  if (vp[2] <= 0 || vp[3] <= 0)
    return 1.0f;
  return static_cast<float>(vp[3]) / static_cast<float>(vp[2]);
}

} // namespace

void reveal_chess_scene::draw_board() {
  m_board_shader->use();
  m_board_shader->set_uniform("u_aspect", viewport_aspect_x());
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

  const bool cheat_reveal_on_hover = (m_connect_mode == connect_mode::none ||
                                      m_connect_mode == connect_mode::server) &&
                                     m_cheat_reveal_all;
  std::vector<int> piece_data;
  for (auto &[r, c] : piece_positions) {
    piece_data.push_back(r);
    piece_data.push_back(c);
    int display_val = m_board[r][c];
    if (cheat_reveal_on_hover && (display_val & piece_type::k_cover_mask))
      display_val = display_val & ~static_cast<int>(piece_type::k_cover_mask);
    piece_data.push_back(display_val);
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
  m_piece_shader->set_uniform("u_aspect", viewport_aspect_x());
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

namespace {
// Match the wood in chess_board/fragment.shader for the invisible hit area
const glm::vec4 k_board_bg = glm::vec4(0.850f, 0.730f, 0.555f, 1.0f);
const glm::vec4 k_hint_color = glm::vec4(0.35f, 0.65f, 0.45f, 0.65f);
const float k_display_radius = 0.03f;
const float k_hit_radius = 0.08f;
} // namespace

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
  m_valid_move_shader->set_uniform("u_aspect", viewport_aspect_x());
  m_valid_move_mesh_manager.bind();

  // Pass 1: large circle — write only to object_id; no color, no depth
  glColorMaski(0, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  glDepthMask(GL_FALSE);
  m_valid_move_shader->set_uniform("u_radius", k_hit_radius);
  m_valid_move_shader->set_uniform("u_fill_color", k_board_bg);
  glDrawArrays(GL_POINTS, 0, m_valid_move_mesh_manager.get_index_count());
  glDepthMask(GL_TRUE);
  glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

  // Pass 2: small green circle — write color and object_id
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  m_valid_move_shader->set_uniform("u_radius", k_display_radius);
  m_valid_move_shader->set_uniform("u_fill_color", k_hint_color);
  glDrawArrays(GL_POINTS, 0, m_valid_move_mesh_manager.get_index_count());
  glDisable(GL_BLEND);
}

namespace {

// Both sides' models come from .env; nothing here names a provider.
void model_note(const char *id, int illegal) {
  ImGui::Text("%s: .env", id);
  ImGui::SameLine();
  ImGui::TextDisabled("illegal moves %d", illegal);
}

} // namespace

void reveal_chess_scene::render() {
  drain_mcp_tasks();
  if (!m_board_shader || !m_piece_shader || !m_valid_move_shader)
    return;
  draw_board();
  draw_pieces();
  draw_text();
  draw_valid_moves();
  draw_last_move();
}

void reveal_chess_scene::render_ui() {
  ImGui::Text("Reveal Chess");
  ImGui::Separator();
  if (m_game_result == game_result::ongoing) {
    if (m_connect_mode != connect_mode::none) {
      if (m_connect_mode == connect_mode::client && !m_board_sync_received)
        ImGui::TextColored(ImVec4(0.7f, 0.6f, 0.2f, 1), "Waiting for board...");
      else if (is_my_turn())
        ImGui::TextColored(ImVec4(0.2f, 0.7f, 0.3f, 1), "Your turn");
      else
        ImGui::TextColored(ImVec4(0.6f, 0.5f, 0.5f, 1), "Opponent's turn");
    } else if (m_chat && m_chat->thinking()) {
      const bool red = m_chat->thinking_side() == "red";
      ImGui::TextColored(red ? ImVec4(0.95f, 0.45f, 0.38f, 1.0f)
                             : ImVec4(0.55f, 0.75f, 0.95f, 1.0f),
                         "%s is thinking  %.1f / %d s", red ? "Red" : "Black",
                         turn_elapsed(), m_move_seconds);
    } else {
      ImGui::Text("Current: %s", m_red_turn ? "Red" : "Black");
    }
  } else if (m_game_result == game_result::red_win) {
    ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.2f, 1.0f), "Red wins!");
  } else {
    ImGui::TextColored(ImVec4(0.2f, 0.2f, 0.3f, 1.0f), "Black wins!");
  }
  ImGui::Text("Hover: (%d, %d)", m_hovered_piece.first, m_hovered_piece.second);
  bool can_restart = (m_connect_mode == connect_mode::none);
  if (!can_restart)
    ImGui::BeginDisabled();
  if (ImGui::Button("Restart")) {
    shuffle_board();
  }
  if (!can_restart) {
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered())
      ImGui::SetTooltip("Disconnect to restart");
  }
  ImGui::SameLine();
  ImGui::Checkbox("Red AI", &m_red_ai);
  ImGui::SameLine();
  ImGui::Checkbox("Black AI", &m_black_ai);
  const bool local = m_connect_mode == connect_mode::none;
  if (m_chat && !local)
    m_chat->stop();
  const bool running = m_chat && m_chat->running();
  if (!local || running)
    ImGui::BeginDisabled();
  model_note("Red model", m_red_illegal);
  model_note("Black model", m_black_illegal);
  ImGui::Checkbox("Think", &m_ai_thinking);
  ImGui::InputInt("Move limit (s)", &m_move_seconds);
  m_move_seconds = std::clamp(m_move_seconds, 5, 3600);
  if (!local || running)
    ImGui::EndDisabled();
  if (running) {
    if (ImGui::Button("Stop"))
      m_chat->stop();
  } else {
    if (!local || !(m_red_ai || m_black_ai))
      ImGui::BeginDisabled();
    if (ImGui::Button("Start") && m_chat) {
      m_chat->start(m_ai_thinking, m_move_seconds, m_log_stamp);
      m_was_thinking = false;
      restart_turn_clock();
    }
    if (!local || !(m_red_ai || m_black_ai))
      ImGui::EndDisabled();
  }
  if (!local)
    ImGui::TextDisabled("AI waits until the network game ends.");
  if (m_chat && (running || m_red_ai || m_black_ai))
    m_chat->draw();

  // ------------------- Cheat
  if (m_connect_mode == connect_mode::none ||
      m_connect_mode == connect_mode::server) {
    ImGui::Separator();
    ImGui::Text("Cheat (Server only when connected)");
    if (piece_index_is_valid(m_hovered_piece)) {
      auto cell = m_board[m_hovered_piece.first][m_hovered_piece.second];
      if (cell != 0) {
        const bool is_read_piece = cell & piece_type::k_red_mask;
        std::string side = is_read_piece ? "Red" : "Black";
        std::string type =
            is_read_piece
                ? piece_text_red.at(static_cast<piece_type>(cell & 0xFF))
                : piece_text_black.at(static_cast<piece_type>(cell & 0xFF));
        ImGui::Text("Hover: Side: %s, Type: %s", side.c_str(), type.c_str());
      }
    }
    ImGui::Button("Reveal All");
    m_cheat_reveal_all = false;
    if (ImGui::IsItemHovered())
      m_cheat_reveal_all = true;
  }

  // Connection
  ImGui::Separator();
  ImGui::Text("Connection");
  if (m_connect_mode == connect_mode::none) {
    ImGui::SliderInt("Port", &m_port, 1024, 65535);
    char host_buf[256];
    strncpy(host_buf, m_host.c_str(), sizeof(host_buf) - 1);
    host_buf[sizeof(host_buf) - 1] = '\0';
    if (ImGui::InputText("Host", host_buf, sizeof(host_buf)))
      m_host = host_buf;
    if (ImGui::Button("Create Server")) {
      if (!m_server_starting && !m_server) {
        m_server_starting = true;
        m_server_ready = false;
        m_server_thread = std::thread([this]() {
          try {
            m_server = new server(static_cast<uint16_t>(m_port));
          } catch (...) {
            m_server = nullptr;
          }
          m_server_ready = true;
          m_server_starting = false;
        });
      }
    }
    if (m_server_starting)
      ImGui::TextColored(ImVec4(1, 0.8f, 0, 1),
                         "Waiting for client on port %d...", m_port);
    if (ImGui::Button("Connect as Client")) {
      if (!m_client) {
        m_client = new client();
        if (m_client->connect(m_host, static_cast<uint16_t>(m_port))) {
          m_connect_mode = connect_mode::client;
          m_client->set_non_blocking(true);
          m_last_recv_time = glfwGetTime();
          m_last_heartbeat_sent_time = m_last_recv_time;
        } else {
          delete m_client;
          m_client = nullptr;
        }
      }
    }
  } else {
    if (m_connect_mode == connect_mode::server) {
      ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.2f, 1), "You are Red (Server)");
      ImGui::Text("Port: %d", m_port);
    } else {
      ImGui::TextColored(ImVec4(0.2f, 0.2f, 0.4f, 1), "You are Black (Client)");
      ImGui::Text("Connected to %s:%d", m_host.c_str(), m_port);
    }
  }
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

namespace {
bool is_red_piece(unsigned int _cell, int _r, int _c) {
  if (_cell == 0)
    return false;
  if ((_cell & piece_type::k_cover_mask) && (_r < 5))
    return true;

  if (!(_cell & piece_type::k_cover_mask) &&
      (_cell & piece_type::k_red_mask) != 0)
    return true;

  return false;
}
bool is_black_piece(unsigned int _cell, int _r, int _c) {
  if (_cell == 0)
    return false;
  if ((_cell & piece_type::k_cover_mask) && (_r >= 5))
    return true;

  if (!(_cell & piece_type::k_cover_mask) &&
      (_cell & piece_type::k_black_mask) != 0)
    return true;

  return false;
}
bool is_piece_of_current_side(bool _red_turn, unsigned int _cell, int _r,
                              int _c) {
  return _red_turn ? is_red_piece(_cell, _r, _c)
                   : is_black_piece(_cell, _r, _c);
}

// Everything below reads only what the diagram shows: occupancy, face-up
// pieces, and the starting square of a covered piece. Never a hidden type.
std::string square_text(int _r, int _c) {
  return "(" + std::to_string(_r) + ", " + std::to_string(_c) + ")";
}

std::string cell_text(unsigned int _cell, int _r, int _c) {
  const bool red = is_red_piece(_cell, _r, _c);
  if (_cell & piece_type::k_cover_mask)
    return red ? "红·" : "黑·";
  const auto type = static_cast<piece_type>(_cell & 0xFF);
  return red ? "红" + piece_text_red.at(type) : "黑" + piece_text_black.at(type);
}

const char *move_type_text(int _type) {
  switch (_type) {
  case piece_type::k_king:
    return "将/帅";
  case piece_type::k_guard:
    return "仕/士";
  case piece_type::k_bishop:
    return "相/象";
  case piece_type::k_rook:
    return "车";
  case piece_type::k_horse:
    return "马";
  case piece_type::k_cannon:
    return "炮";
  case piece_type::k_soldier:
    return "兵/卒";
  default:
    return "?";
  }
}

// Pieces strictly between two squares on one row or column.
int pieces_between(const int _board[10][9], int _fr, int _fc, int _tr, int _tc,
                   std::pair<int, int> *_first) {
  const int dr = (_tr > _fr) - (_tr < _fr), dc = (_tc > _fc) - (_tc < _fc);
  int count = 0;
  for (int r = _fr + dr, c = _fc + dc; r != _tr || c != _tc; r += dr, c += dc) {
    if (_board[r][c] == 0)
      continue;
    if (count++ == 0 && _first)
      *_first = {r, c};
  }
  return count;
}

std::string why_illegal(const int _board[10][9], int _fr, int _fc, int _tr,
                        int _tc) {
  const unsigned int cell = static_cast<unsigned int>(_board[_fr][_fc]);
  const unsigned int target = static_cast<unsigned int>(_board[_tr][_tc]);
  const bool covered = (cell & piece_type::k_cover_mask) != 0;
  const int type = covered ? default_type_at(_fr, _fc) : (cell & 0xFF);
  const int dr = _tr - _fr, dc = _tc - _fc;
  const int adr = std::abs(dr), adc = std::abs(dc);
  if (dr == 0 && dc == 0)
    return "The destination is the source square.";
  if (target != 0 && is_friend(static_cast<int>(target), static_cast<int>(cell)))
    return square_text(_tr, _tc) + " holds your own face-up " +
           cell_text(target, _tr, _tc) + ", which you cannot capture.";
  switch (type) {
  case piece_type::k_rook:
  case piece_type::k_cannon: {
    if (dr != 0 && dc != 0)
      return std::string("A ") + move_type_text(type) +
             " moves only along one row or one column.";
    std::pair<int, int> first{-1, -1};
    const int between = pieces_between(_board, _fr, _fc, _tr, _tc, &first);
    if (type == piece_type::k_rook || target == 0) {
      if (between > 0)
        return "The path is blocked at " +
               square_text(first.first, first.second) + ".";
      break;
    }
    if (between != 1)
      return "A 炮 captures only by jumping exactly one piece, and there " +
             std::string(between == 0 ? "is none" : "are " + std::to_string(between)) +
             " between.";
    break;
  }
  case piece_type::k_horse: {
    if (!((adr == 2 && adc == 1) || (adr == 1 && adc == 2)))
      return "A 马 moves two rows and one column, or one row and two columns.";
    const int leg_r = adr == 2 ? _fr + dr / 2 : _fr;
    const int leg_c = adc == 2 ? _fc + dc / 2 : _fc;
    if (_board[leg_r][leg_c] != 0)
      return "The 马 is blocked: its leg square " + square_text(leg_r, leg_c) +
             " is occupied.";
    break;
  }
  case piece_type::k_bishop:
    if (adr != 2 || adc != 2)
      return "A 相/象 moves exactly two squares diagonally.";
    if (_board[_fr + dr / 2][_fc + dc / 2] != 0)
      return "The 相/象 is blocked: the middle square " +
             square_text(_fr + dr / 2, _fc + dc / 2) + " is occupied.";
    break;
  case piece_type::k_guard:
    if (adr != 1 || adc != 1)
      return "A 仕/士 moves exactly one square diagonally.";
    if (covered)
      return "A covered piece on a 仕/士 starting square may only step to " +
             square_text(_fr == 0 ? 1 : 8, 4) + ".";
    break;
  case piece_type::k_king:
    if (adr + adc != 1)
      return "A king moves exactly one square along a row or column.";
    return "A king must stay inside its palace: rows 0–2 for Red, rows 7–9 "
           "for Black, columns 3–5.";
  case piece_type::k_soldier: {
    const int forward = is_red_piece(cell, _fr, _fc) ? 1 : -1;
    if (dr == forward && dc == 0)
      break;
    if (dr == 0 && adc == 1)
      return covered ? "A covered piece on a 兵/卒 starting square only steps "
                       "forward."
                     : "A 兵/卒 may step sideways only after crossing the "
                       "river: rows 5–9 for a red one, rows 0–4 for a black "
                       "one.";
    return std::string("This 兵/卒 steps one square forward, toward row ") +
           (forward > 0 ? "9" : "0") + ", and never backward or diagonally.";
  }
  default:
    break;
  }
  return "That piece cannot reach that square.";
}

// A machine-readable reason for a refused move, alongside the prose from
// explain_failure. The prose is for the model; this is for counting, so
// rewording the prose never changes the statistics.
std::string failure_reason(const std::string &_error, const int _board[10][9],
                           int _fr, int _fc) {
  if (_error == "That piece is not the side to move.")
    return _board[_fr][_fc] == 0 ? "empty" : "not_your_piece";
  if (_error == "Kings would face.")
    return "facing";
  if (_error == "Undeclared check.")
    return "undeclared_check";
  if (_error == "Illegal move.")
    return "geometry";
  if (_error == "The game is over.")
    return "game_over";
  if (_error == "Square is off the board.")
    return "off_board";
  return "other";
}

std::string explain_failure(const std::string &_error, const int _board[10][9],
                            bool _red_turn, int _fr, int _fc, int _tr, int _tc) {
  const unsigned int cell = static_cast<unsigned int>(_board[_fr][_fc]);
  const std::string side = _red_turn ? "Red" : "Black";
  const std::string yours = _red_turn
                                ? "face-up 红 pieces and covered 红· on rows 0–4"
                                : "face-up 黑 pieces and covered 黑· on rows 5–9";
  if (_error == "That piece is not the side to move.") {
    const std::string what =
        cell == 0 ? square_text(_fr, _fc) + " is empty."
                  : square_text(_fr, _fc) + " holds " +
                        cell_text(cell, _fr, _fc) + ", which is not yours.";
    return what + " You are " + side + ". You may move " + yours +
           ". A covered square shows which half it stands on, not whose piece "
           "it is, until it is revealed. Re-read the diagram: the first field "
           "of a row is the row number, then columns 0 through 8.";
  }
  if (_error == "Kings would face.") {
    return "Illegal: " + cell_text(cell, _fr, _fc) + " " +
           square_text(_fr, _fc) + " to " + square_text(_tr, _tc) +
           ". That would leave the two kings on the same open line, which "
           "xiangqi never allows: the side to move may not expose them to each "
           "other. Any piece between them blocks the line, a covered piece "
           "included. Block the line or move the king; the board did not "
           "change.";
  }
  if (_error == "Undeclared check.") {
    return "Illegal: taking the king at " + square_text(_tr, _tc) +
           " needs a check you gave on your own previous move. Your last move "
           "did not check that side, so the engine never announced it and the "
           "capture is not allowed. Play a different move; the board did not "
           "change.";
  }
  if (_error != "Illegal move.")
    return _error;

  std::string text = "Illegal: " + cell_text(cell, _fr, _fc) + " " +
                     square_text(_fr, _fc) + " to " + square_text(_tr, _tc) +
                     ". " + why_illegal(_board, _fr, _fc, _tr, _tc);
  if (cell & piece_type::k_cover_mask)
    text += std::string(" It is covered and stands on a ") +
            move_type_text(default_type_at(_fr, _fc)) +
            " starting square, so it moves as that piece.";
  const auto moves = legal_moves_for_piece(
      _board, static_cast<piece_type>(cell), _fr, _fc);
  if (moves.empty()) {
    text += " This piece has no legal move now. Pick a different piece.";
  } else {
    text += " This piece can go to:";
    for (std::size_t i = 0; i < moves.size(); ++i)
      text += (i == 0 ? " " : ", ") +
              square_text(moves[i].first, moves[i].second);
    text += ". Pick one of these only if it is safe, or move another piece.";
  }
  return text;
}
} // namespace

bool reveal_chess_scene::commit_move(int fr, int fc, int tr, int tc,
                                     std::string *error) {
  auto fail = [&](const char *msg) {
    if (error)
      *error = msg;
    return false;
  };
  if (m_game_result != game_result::ongoing)
    return fail("The game is over.");
  if (!in_bounds(fr, fc) || !in_bounds(tr, tc))
    return fail("Square is off the board.");
  const unsigned int cell = static_cast<unsigned int>(m_board[fr][fc]);
  if (!is_piece_of_current_side(m_red_turn, cell, fr, fc))
    return fail("That piece is not the side to move.");
  const unsigned int captured = static_cast<unsigned int>(m_board[tr][tc]);
  const auto moves = get_valid_moves_for_piece(
      m_board, static_cast<piece_type>(cell), fr, fc);
  if (std::find(moves.begin(), moves.end(), std::pair<int, int>{tr, tc}) ==
      moves.end())
    return fail("Illegal move.");
  // Checked after the geometry, so an unreachable square is still explained as
  // unreachable rather than as the general rule.
  if (leaves_kings_facing(m_board, fr, fc, tr, tc))
    return fail("Kings would face.");
  // A king is only winnable through a check this side gave itself. The engine
  // announces every check, so a capture the opponent was never warned about is
  // a capture of a king that was simply left hanging.
  const bool takes_king =
      captured != 0 && (captured & 0xFF) == piece_type::k_king;
  if (takes_king && !(m_red_turn ? m_red_check_given : m_black_check_given))
    return fail("Undeclared check.");

  const bool moved_by_red = m_red_turn;
  record_move(fr, fc, tr, tc, cell, captured, moved_by_red);
  m_board[tr][tc] = static_cast<int>(cell & ~piece_type::k_cover_mask);
  m_board[fr][fc] = 0;
  m_last_move_from = {fr, fc};
  m_last_move_to = {tr, tc};
  invalidate_piece_index(m_selected_piece);
  if (m_connect_mode != connect_mode::none)
    send_move(fr, fc, tr, tc);
  if (captured != 0 && (captured & 0xFF) == piece_type::k_king)
    m_game_result = moved_by_red ? game_result::red_win : game_result::black_win;
  // Recorded after the board changed: this is the check the opponent will be
  // told about, and the one that licenses taking their king next turn.
  if (moved_by_red)
    m_red_check_given = side_checks(m_board, true);
  else
    m_black_check_given = side_checks(m_board, false);
  if (m_game_result == game_result::ongoing)
    m_red_turn = !m_red_turn;
  restart_turn_clock();
  return true;
}

std::string reveal_chess_scene::board_json() {
  using nlohmann::json;
  // Markdown table. 红马 and 黑马 stay distinct: 车马炮 share a name
  // across sides, and the leading 红 or 黑 is the side.
  std::string diagram = "| | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |\n";
  diagram += "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |\n";
  bool can_move = false;
  for (int r = 0; r < 10; ++r) {
    diagram += "| " + std::to_string(r) + " |";
    for (int c = 0; c < 9; ++c) {
      const unsigned int cell = static_cast<unsigned int>(m_board[r][c]);
      const bool red = cell != 0 && is_red_piece(cell, r, c);
      diagram += " ";
      if (cell != 0 && (cell & piece_type::k_cover_mask) != 0) {
        diagram += red ? "红·" : "黑·";
      } else if (cell != 0) {
        diagram += red ? "红" : "黑";
        diagram += red ? piece_text_red.at(static_cast<piece_type>(cell & 0xFF))
                       : piece_text_black.at(static_cast<piece_type>(cell & 0xFF));
      }
      diagram += " |";
      if (!can_move && cell != 0 &&
          is_piece_of_current_side(m_red_turn, cell, r, c) &&
          !legal_moves_for_piece(m_board, static_cast<piece_type>(cell), r, c)
               .empty())
        can_move = true;
    }
    diagram += '\n';
  }
  json doc{{"turn", m_red_turn ? "red" : "black"},
           {"result", result_name(m_game_result)},
           {"ply", m_ply},
           {"can_move", can_move},
           {"red_ai", m_red_ai},
           {"black_ai", m_black_ai},
           {"mode", m_connect_mode == connect_mode::none ? "local" : "network"},
           {"diagram", std::move(diagram)}};
  // The announcement: which sides are in check right now. Published here so the
  // side to move learns it from the engine, not from the opponent's own words,
  // which nobody else sees.
  const bool red_in_check = side_checks(m_board, false);
  const bool black_in_check = side_checks(m_board, true);
  json in_check = json::array();
  if (red_in_check)
    in_check.push_back("red");
  if (black_in_check)
    in_check.push_back("black");
  doc["check"] = std::move(in_check);
  // A king is only winnable after a check its taker gave on its own previous
  // move, and only while that king is still under attack.
  const bool checked_last = m_red_turn ? m_red_check_given : m_black_check_given;
  doc["may_take_king"] =
      checked_last && (m_red_turn ? black_in_check : red_in_check);
  if (piece_index_is_valid(m_last_move_from)) {
    doc["last_move"] = {{"from_row", m_last_move_from.first},
                        {"from_col", m_last_move_from.second},
                        {"to_row", m_last_move_to.first},
                        {"to_col", m_last_move_to.second}};
  }
  return doc.dump();
}

std::string reveal_chess_scene::clock_json() {
  using nlohmann::json;
  const double elapsed = turn_elapsed();
  return json{{"limit", m_move_seconds},
              {"elapsed", elapsed},
              {"seconds_left", m_move_seconds - elapsed}}
      .dump();
}

std::string reveal_chess_scene::moves_json(int row, int col) {
  using nlohmann::json;
  // Every call costs, refusals included; otherwise asking about empty or
  // foreign squares would be a free sweep of the board.
  if (m_move_queries >= k_reveal_chess_move_queries)
    return json{{"ok", false},
                {"error", "No move queries left this turn (limit " +
                              std::to_string(k_reveal_chess_move_queries) +
                              "). Decide from what you already know."},
                {"queries_left", 0}}
        .dump();
  const int left = k_reveal_chess_move_queries - ++m_move_queries;
  const auto refuse = [left](const std::string &_error) {
    return json{{"ok", false}, {"error", _error}, {"queries_left", left}}.dump();
  };
  if (m_game_result != game_result::ongoing)
    return refuse("The game is over.");
  if (!in_bounds(row, col))
    return refuse("Square is off the board.");
  const unsigned int cell = static_cast<unsigned int>(m_board[row][col]);
  if (cell == 0)
    return refuse(square_text(row, col) + " is empty; there is no piece to ask about.");
  if (!is_piece_of_current_side(m_red_turn, cell, row, col))
    return refuse(square_text(row, col) + " holds " + cell_text(cell, row, col) +
                  ", which is not yours this turn, so it cannot move now.");

  const auto squares = legal_moves_for_piece(
      m_board, static_cast<piece_type>(cell), row, col);
  json reply{{"ok", true},
             {"from_row", row},
             {"from_col", col},
             {"piece", cell_text(cell, row, col)},
             {"covered", (cell & piece_type::k_cover_mask) != 0},
             {"moves", json::array()},
             {"queries_left", left}};
  for (const auto &square : squares)
    reply["moves"].push_back({square.first, square.second});
  return reply.dump();
}

void reveal_chess_scene::restart_turn_clock() {
  m_turn_started = std::chrono::steady_clock::now();
  m_move_queries = 0;
}

double reveal_chess_scene::turn_elapsed() const {
  return std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                      m_turn_started)
      .count();
}

void reveal_chess_scene::enforce_clock() {
  // The agent cancels a late request and retries once without thinking, so a
  // side only loses after missing the budget twice. A checked AI box with no
  // running agent must not end the game.
  if (m_game_result != game_result::ongoing ||
      m_connect_mode != connect_mode::none || !m_chat || !m_chat->running() ||
      !(m_red_turn ? m_red_ai : m_black_ai))
    return;
  if (turn_elapsed() <= 2.0 * m_move_seconds + k_move_grace)
    return;
  m_game_result = m_red_turn ? game_result::black_win : game_result::red_win;
  m_chat->note("error", "超时判负", m_red_turn ? "red" : "black");
}

std::string reveal_chess_scene::play_move(int from_row, int from_col, int to_row,
                                          int to_col, std::string_view note) {
  using nlohmann::json;
  const bool side_ai = m_red_turn ? m_red_ai : m_black_ai;
  if (!side_ai)
    return json{{"ok", false},
                {"error", "It is the human's turn."},
                {"reason", "human_turn"}}
        .dump();
  if (m_connect_mode != connect_mode::none)
    return json{{"ok", false},
                {"error", "A network game is in progress."},
                {"reason", "network"}}
        .dump();
  if (note.find_first_not_of(" \t\r\n") == std::string_view::npos)
    return json{{"ok", false},
                {"error", "note is empty. Pass one or two Chinese sentences on "
                          "why this move; the board did not change."},
                {"reason", "note_missing"}}
        .dump();
  if (!in_bounds(from_row, from_col) || !in_bounds(to_row, to_col)) {
    ++(m_red_turn ? m_red_illegal : m_black_illegal);
    return json{{"ok", false},
                {"error", "Square is off the board."},
                {"reason", "off_board"}}
        .dump();
  }

  const unsigned int target = static_cast<unsigned int>(m_board[to_row][to_col]);
  std::string error;
  const bool red_moved = m_red_turn;
  if (!commit_move(from_row, from_col, to_row, to_col, &error)) {
    const std::string reason = failure_reason(error, m_board, from_row, from_col);
    if (error != "The game is over.")
      ++(m_red_turn ? m_red_illegal : m_black_illegal);
    return json{{"ok", false},
                {"reason", reason},
                {"error", explain_failure(error, m_board, m_red_turn, from_row,
                                          from_col, to_row, to_col)}}
        .dump();
  }

  json reply{{"ok", true},
             {"from_row", from_row},
             {"from_col", from_col},
             {"to_row", to_row},
             {"to_col", to_col},
             {"result", result_name(m_game_result)}};
  // The check this move just gave, if any. The mover sees it at once; the
  // opponent reads the same fact from the board on its turn.
  const bool checked = side_checks(m_board, red_moved);
  if (checked)
    reply["check"] = red_moved ? "black" : "red";
  // A covered mover is face up on the next board call. A covered capture leaves
  // the board without ever showing its type, so that type stays hidden here too.
  if (target != 0) {
    reply["captured"] = (target & piece_type::k_cover_mask) != 0
                            ? "covered"
                            : piece_name(target);
  }
  return reply.dump();
}

std::string
reveal_chess_scene::on_render_thread(std::function<std::string()> fn) {
  auto prom = std::make_shared<std::promise<std::string>>();
  const std::uint64_t id = m_mcp_seq.fetch_add(1);
  {
    std::lock_guard<std::mutex> lock(m_mcp_mu);
    m_mcp_tasks.push_back(mcp_task{
        id, [fn = std::move(fn), prom]() mutable {
          try {
            prom->set_value(fn());
          } catch (...) {
            prom->set_exception(std::current_exception());
          }
        }});
  }
  auto fut = prom->get_future();
  if (fut.wait_for(std::chrono::seconds(3)) != std::future_status::ready) {
    std::lock_guard<std::mutex> lock(m_mcp_mu);
    std::erase_if(m_mcp_tasks,
                  [id](const mcp_task &task) { return task.id == id; });
  }
  if (fut.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
    throw std::runtime_error(
        "Reveal Chess is not rendering. Select it in Scene Selection.");
  }
  return fut.get();
}

void reveal_chess_scene::record_move(int fr, int fc, int tr, int tc,
                                     unsigned int mover, unsigned int target,
                                     bool red_moved) {
  const std::string path = truth_path();
  if (path.empty())
    return;
  using nlohmann::json;
  const auto truth = [](unsigned int _cell) {
    return json{{"covered", (_cell & piece_type::k_cover_mask) != 0},
                {"color", (_cell & piece_type::k_black_mask) != 0 ? "black"
                                                                  : "red"},
                {"piece", piece_name(_cell)}};
  };
  json record{{"ply", ++m_ply},
              {"side", red_moved ? "red" : "black"},
              {"from_row", fr},
              {"from_col", fc},
              {"to_row", tr},
              {"to_col", tc},
              {"mover", truth(mover)},
              {"target", target == 0 ? json(nullptr) : truth(target)}};
  std::ofstream out(path, std::ios::app);
  if (out)
    out << record.dump() << '\n';
}

void reveal_chess_scene::forget_history() {
  m_ply = 0;
  // One stamp per match: the board is new, so the log is too.
  char stamp[32] = {};
  const std::time_t now = std::time(nullptr);
  std::strftime(stamp, sizeof(stamp), "%Y-%m-%d_%H-%M-%S", std::localtime(&now));
  m_log_stamp = stamp;
  std::error_code ec;
  std::filesystem::create_directories(
      std::filesystem::path(".output") / "chess", ec);
}

std::string reveal_chess_scene::truth_path() const {
  if (m_log_stamp.empty())
    return {};
  return (std::filesystem::path(".output") / "chess" /
          (m_log_stamp + ".truth.jsonl"))
      .string();
}

void reveal_chess_scene::drain_mcp_tasks() {
  std::vector<mcp_task> tasks;
  {
    std::lock_guard<std::mutex> lock(m_mcp_mu);
    tasks.swap(m_mcp_tasks);
  }
  for (auto &task : tasks)
    task.run();
}

bool reveal_chess_scene::on_mouse_button(int _button, int _action, int _mods) {
  if (_button != GLFW_MOUSE_BUTTON_LEFT || _action != GLFW_PRESS)
    return false;
  if (m_game_result != game_result::ongoing)
    return true;
  if (m_connect_mode != connect_mode::none && !is_my_turn())
    return true;
  if (m_connect_mode == connect_mode::none &&
      (m_red_turn ? m_red_ai : m_black_ai))
    return true;

  if (piece_index_is_valid(m_selected_piece)) {
    if (!piece_index_is_valid(m_hovered_piece)) {
      invalidate_piece_index(m_selected_piece);
      return true;
    }
    if (m_selected_piece == m_hovered_piece) {
      invalidate_piece_index(m_selected_piece);
      return true;
    }
    auto valid_moves = get_valid_moves();
    auto iter =
        std::find(valid_moves.begin(), valid_moves.end(), m_hovered_piece);
    if (iter == valid_moves.end()) {
      // Clicked on non-valid cell: select it only if it's our piece
      int r = m_hovered_piece.first, c = m_hovered_piece.second;
      if (is_piece_of_current_side(m_red_turn, m_board[r][c], r, c))
        m_selected_piece = m_hovered_piece;
      else
        invalidate_piece_index(m_selected_piece);
      return true;
    }
    commit_move(m_selected_piece.first, m_selected_piece.second,
                m_hovered_piece.first, m_hovered_piece.second, nullptr);
    return true;
  } else {
    // No selection: select only if hovered cell has current side's piece
    if (!piece_index_is_valid(m_hovered_piece)) {
      return true;
    }
    int r = m_hovered_piece.first, c = m_hovered_piece.second;
    if (is_piece_of_current_side(m_red_turn, m_board[r][c], r, c))
      m_selected_piece = m_hovered_piece;
  }
  return true;
}

std::vector<std::pair<int, int>> reveal_chess_scene::get_valid_moves() {
  if (m_selected_piece.first == -1 || m_selected_piece.second == -1)
    return {};

  int r = m_selected_piece.first, c = m_selected_piece.second;
  unsigned int cell = m_board[r][c];

  return legal_moves_for_piece(m_board, static_cast<piece_type>(cell), r, c);
}

void reveal_chess_scene::draw_text() {
  const float aspect = viewport_aspect_x();

  // 楚河汉界, on the river. Drawn before the pieces and further from the camera,
  // so a piece standing on the bank covers it.
  {
    const float scale =
        interaction_utils::viewport_scale_from_ref_height(800.0f) * 0.62f;
    const glm::vec3 ink(0.330f, 0.215f, 0.120f);
    const char *labels[2] = {"楚 河", "汉 界"};
    const float centers[2] = {-0.42f, 0.42f};
    for (int i = 0; i < 2; ++i) {
      float w_ndc = 0, h_ndc = 0, bearing_y_ndc = 0;
      text_renderer::instance().measure_text_ndc(labels[i], scale, &w_ndc,
                                                 &h_ndc, &bearing_y_ndc);
      text_renderer::instance().render_text_by_uv(
          labels[i], centers[i] * aspect - w_ndc * 0.5f, -h_ndc * 0.5f, scale,
          ink, -0.04f);
    }
  }

  for (int r = 0; r < 10; r++) {
    for (int c = 0; c < 9; c++) {

      const bool is_hovered = m_hovered_piece == std::pair<int, int>(r, c);
      const bool is_selected = m_selected_piece == std::pair<int, int>(r, c);
      if (m_board[r][c]) {
        if (m_board[r][c] & piece_type::k_cover_mask) {
          const bool cheat_show = (m_connect_mode == connect_mode::none ||
                                   m_connect_mode == connect_mode::server) &&
                                  m_cheat_reveal_all;
          if (!cheat_show)
            continue;
        }

        std::string piece_text;
        glm::vec3 text_color;
        if (m_board[r][c] & piece_type::k_red_mask) {
          piece_text =
              piece_text_red.at(static_cast<piece_type>(m_board[r][c] & 0xFF));
          text_color = glm::vec3(0.98f, 0.94f, 0.72f); // warm cream/gold on red
        } else {
          piece_text = piece_text_black.at(
              static_cast<piece_type>(m_board[r][c] & 0xFF));
          text_color = glm::vec3(0.98f, 0.96f, 0.92f); // warm white on black
        }

        auto mix = [](float _a, float _b, float _t) {
          return _a + (_b - _a) * _t;
        };

        const float ref_height = 800.0f;
        float scale =
            interaction_utils::viewport_scale_from_ref_height(ref_height);
        float cx = mix(-0.9f, 0.9f, float(c) / 8.0f) * aspect;
        float cy = mix(0.9f, -0.9f, float(r) / 9.0f);

        if (is_hovered) {
          cy += 0.012f; // matches k_hover_lift in chess_piece/geometry.shader
        }

        float w_ndc = 0, h_ndc = 0, bearing_y_ndc = 0;
        text_renderer::instance().measure_text_ndc(piece_text, scale, &w_ndc,
                                                   &h_ndc, &bearing_y_ndc);
        float origin_x = cx - w_ndc * 0.5f;
        // float baseline = cy - h_ndc * 0.5f + bearing_y_ndc;
        float baseline = cy - h_ndc * 0.5f;

        text_renderer::instance().render_text_by_uv(
            piece_text, origin_x, baseline, scale, text_color, -0.25f);
      }
    }
  }
}

void reveal_chess_scene::draw_last_move() {
  if (!piece_index_is_valid(m_last_move_from) ||
      !piece_index_is_valid(m_last_move_to))
    return;
  if (!m_last_move_shader)
    return;

  std::vector<int> cell_data;
  cell_data.push_back(m_last_move_from.first);
  cell_data.push_back(m_last_move_from.second);
  cell_data.push_back(m_last_move_to.first);
  cell_data.push_back(m_last_move_to.second);
  mesh_data last_move_mesh(cell_data.data(), cell_data.size() * sizeof(int), 2,
                           {{1, GL_INT, GL_FALSE}, {1, GL_INT, GL_FALSE}});
  m_last_move_mesh_manager.setup_mesh(last_move_mesh);

  m_last_move_shader->use();
  m_last_move_shader->set_uniform("u_aspect", viewport_aspect_x());
  m_last_move_mesh_manager.bind();
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  m_last_move_shader->set_uniform(
      "u_color", glm::vec4(0.95f, 0.85f, 0.45f, 0.75f)); // warm yellow bracket
  glDrawArrays(GL_POINTS, 0, m_last_move_mesh_manager.get_index_count());
  glDisable(GL_BLEND);
}