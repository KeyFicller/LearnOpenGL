#include "reveal_chess_scene.h"

#include "glad/gl.h"
#include "glm/fwd.hpp"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <arpa/inet.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>

#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_quad.h"
#include "tests/component/text_renderer.h"
#include "tests/component/interaction_utils.h"

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

bool piece_index_is_valid(const std::pair<int, int> &_index) {
  return _index.first != -1 && _index.second != -1;
}

void invalidate_piece_index(std::pair<int, int> &_index) { _index = {-1, -1}; }

} // namespace

reveal_chess_scene::reveal_chess_scene() : test_scene_base("Reveal Chess") {
  std::memset(m_board, 0, sizeof(m_board));
}

reveal_chess_scene::~reveal_chess_scene() {
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
  m_board[tr][tc] = piece & (~piece_type::k_cover_mask);
  m_board[fr][fc] = 0;
  m_last_move_from = {fr, fc};
  m_last_move_to = {tr, tc};
  invalidate_piece_index(m_selected_piece);

  if (captured != 0) {
    piece_type pt = static_cast<piece_type>(captured & 0xFF);
    if (pt == piece_type::k_king)
      m_game_result =
          m_red_turn ? game_result::red_win : game_result::black_win;
  }
  if (m_game_result == game_result::ongoing)
    m_red_turn = !m_red_turn;
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
  if (m_server->send(buf, sizeof(buf)) == static_cast<ssize_t>(sizeof(buf)))
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
  ssize_t n = -1;
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
// Match board background in chess_board/fragment.shader for invisible hit area
const glm::vec4 k_board_bg = glm::vec4(0.82f, 0.71f, 0.55f, 1.0f);
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

void reveal_chess_scene::render() {
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
} // namespace

bool reveal_chess_scene::on_mouse_button(int _button, int _action, int _mods) {
  if (_button != GLFW_MOUSE_BUTTON_LEFT || _action != GLFW_PRESS)
    return false;
  if (m_game_result != game_result::ongoing)
    return true;
  if (m_connect_mode != connect_mode::none && !is_my_turn())
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
    // Execute move
    int tr = m_hovered_piece.first, tc = m_hovered_piece.second;
    unsigned int captured = m_board[tr][tc];
    auto sel_piece_type =
        m_board[m_selected_piece.first][m_selected_piece.second];

    m_board[tr][tc] = sel_piece_type & (~piece_type::k_cover_mask);
    m_board[m_selected_piece.first][m_selected_piece.second] = 0;

    m_last_move_from = m_selected_piece;
    m_last_move_to = m_hovered_piece;
    invalidate_piece_index(m_selected_piece);

    if (m_connect_mode != connect_mode::none)
      send_move(m_last_move_from.first, m_last_move_from.second,
                m_last_move_to.first, m_last_move_to.second);

    // Victory: captured enemy king
    if (captured != 0) {
      piece_type pt = static_cast<piece_type>(captured & 0xFF);
      if (pt == piece_type::k_king)
        m_game_result =
            m_red_turn ? game_result::red_win : game_result::black_win;
    }
    if (m_game_result == game_result::ongoing)
      m_red_turn = !m_red_turn;
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

  return get_valid_moves_for_piece(m_board, static_cast<piece_type>(cell), r,
                                   c);
}

void reveal_chess_scene::draw_text() {
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
        float cx = mix(-0.9f, 0.9f, float(c) / 8.0f);
        float cy = mix(0.9f, -0.9f, float(r) / 9.0f);

        if (is_hovered) {
          cy += 0.01f * 0.9f;
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
  m_last_move_mesh_manager.bind();
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  m_last_move_shader->set_uniform(
      "u_color", glm::vec4(0.95f, 0.85f, 0.45f, 0.75f)); // warm yellow bracket
  glDrawArrays(GL_POINTS, 0, m_last_move_mesh_manager.get_index_count());
  glDisable(GL_BLEND);
}