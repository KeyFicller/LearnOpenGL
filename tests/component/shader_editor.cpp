#include "shader_editor.h"
#include "TextEditor.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstddef>

namespace {
std::string seek_for_seed(const std::string &_current_line, int _index,
                          int *_stop_index = nullptr) {
  if (_current_line.empty() || _index == 0) {
    return "";
  }

  int seed_start = _index;
  while (seed_start > 0 &&
         seed_start <= static_cast<int>(_current_line.length()) &&
         (std::isalnum(
              static_cast<unsigned char>(_current_line[seed_start - 1])) ||
          _current_line[seed_start - 1] == '_')) {
    seed_start--;
  }

  if (seed_start < _index) {
    if (_stop_index) {
      *_stop_index = seed_start;
    }
    return _current_line.substr(seed_start, _index - seed_start);
  }

  return "";
}

// https://en.wikipedia.org/wiki/UTF-8
// We assume that the char is a standalone character (<128) or a leading byte of
// an UTF-8 code sequence (non-10xxxxxx code)
static int UTF8CharLength(TextEditor::Char c) {
  if ((c & 0xFE) == 0xFC)
    return 6;
  if ((c & 0xFC) == 0xF8)
    return 5;
  if ((c & 0xF8) == 0xF0)
    return 4;
  else if ((c & 0xF0) == 0xE0)
    return 3;
  else if ((c & 0xE0) == 0xC0)
    return 2;
  return 1;
}
}; // namespace

basic_code_editor::basic_code_editor(const std::string &_name,
                                     code_editor_type _type)
    : m_name(_name) {
  switch (_type) {
  case code_editor_type::k_shader:
    m_editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
    break;
  case code_editor_type::k_script:
    m_editor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
    break;
  default:
    break;
  }

  // Bind some user defined callbacks.
  m_editor.SetKeyPressedCallback(
      std::bind(&basic_code_editor::key_pressed_events_entry, this));

  m_editor.SetMouseScrolledCallback(
      std::bind(&basic_code_editor::mouse_scrolled_events_entry, this));
}

void basic_code_editor::set_text(const std::string &_text) {
  m_editor.SetText(_text);
}

std::string basic_code_editor::get_text() const { return m_editor.GetText(); }

void basic_code_editor::set_help_info(const std::string &_hint) {
  m_help_info = _hint;
}

autocomplete_seed basic_code_editor::seed_for_autocomplete() const {
  return autocomplete_seed{autocomplete_type::k_keyword, "",
                           seek_for_seed(m_editor.GetCurrentLineText(),
                                         m_editor.GetCursorPosition().mColumn)};
}

void basic_code_editor::render() {
  auto cpos = m_editor.GetCursorPosition();
  ImGui::Begin(m_name.c_str());
  ImGui::TextDisabled("%s", m_help_info.c_str());

  ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s", cpos.mLine + 1,
              cpos.mColumn + 1, m_editor.GetTotalLines(),
              m_editor.IsOverwrite() ? "Ovr" : "Ins",
              m_editor.CanUndo() ? "*" : " ",
              m_editor.GetLanguageDefinition().mName.c_str());
  ImGui::PushFont(NULL, m_font_scale * ImGui::GetFontSize());
  m_editor.Render((m_name + "Impl").c_str());

  if (m_editor.IsTextChanged() && !m_editor.LastOperationIsDelete()) {
    if (m_just_inserted_completion) {
      m_just_inserted_completion = false;
      ImGui::PopFont();
      ImGui::End();
      return;
    }
    update_candidates();
  }

  if (m_show_autocomplete_pooup && !m_autocomplete_candidates.empty())
    show_autocomplete_popup();

  if (m_editor.IsTextChanged())
    restore_default_help_info();
  ImGui::PopFont();

  ImGui::End();
}

void basic_code_editor::set_tab_to_indent(bool _tab_to_indent) {
  m_tab_to_indent = _tab_to_indent;
}

void basic_code_editor::format_text() {}

void basic_code_editor::restore_default_help_info() {
  if (m_save_callback)
    m_help_info = "Ctrl+S to save.";
  else
    m_help_info = "Enjoy editing!";
}

void basic_code_editor::update_candidates() {
  auto seed = seed_for_autocomplete();
  if (seed != m_cached_seed) {
    m_cached_seed = seed;
    if (!need_search_for_candidates()) {
      m_show_autocomplete_pooup = false;
      m_autocomplete_candidates.clear();
      return;
    }
  } else {
    return;
  }

  scan_for_context();
  m_autocomplete_candidates.clear();

  if (seed.type == autocomplete_type::k_keyword) {
    auto &builtin_keywords = get_builtin_keywords();
    for (const auto &keyword : builtin_keywords) {
      if (keyword.length() >= seed.prefix.length() &&
          keyword.substr(0, seed.prefix.length()) == seed.prefix) {
        m_autocomplete_candidates.push_back(keyword);
      }
    }
  }
  auto &defined_keywords = get_defined_keywords(m_cached_seed);
  if (defined_keywords.find(seed.type) != defined_keywords.end()) {
    for (const auto &keyword : defined_keywords.at(seed.type)) {
      if (seed.prefix.empty())
        m_autocomplete_candidates.push_back(keyword);
      else if (keyword.length() >= seed.prefix.length() &&
               keyword.substr(0, seed.prefix.length()) == seed.prefix) {
        m_autocomplete_candidates.push_back(keyword);
      }
    }
  }

  std::sort(m_autocomplete_candidates.begin(), m_autocomplete_candidates.end());
  m_selected_candidate = 0;

  m_show_autocomplete_pooup = !m_autocomplete_candidates.empty();
  if (m_autocomplete_candidates.size() == 1 &&
      m_autocomplete_candidates[0] == seed.prefix) {
    m_show_autocomplete_pooup = false;
  }
}

bool basic_code_editor::need_search_for_candidates() const {
  return m_cached_seed.type != autocomplete_type::k_none &&
         !(m_cached_seed.type == autocomplete_type::k_keyword &&
           m_cached_seed.prefix.empty());
}

void basic_code_editor::show_autocomplete_popup() {
  if (m_autocomplete_candidates.empty()) {
    return;
  }

  ImVec2 window_pos = ImGui::GetWindowPos();
  ImVec2 content_min = ImGui::GetWindowContentRegionMin();

  auto cursor_pos = m_editor.GetCursorPosition();

  int total_lines = m_editor.GetTotalLines();
  float left_margin =
      ImGui::CalcTextSize(
          std::string("0000" + std::to_string(total_lines)).c_str())
          .x;

  auto line = m_editor.GetCurrentLineText();
  std::string line_before = line.substr(0, cursor_pos.mColumn);
  float left_offset = ImGui::CalcTextSize(line_before.c_str()).x;

  float line_height = ImGui::GetTextLineHeight();
  float up_margin = line_height * 2.0f;

  float up_offset = (cursor_pos.mLine + 2) * line_height;

  ImVec2 popup_pos =
      ImVec2(window_pos.x + content_min.x + left_margin + left_offset,
             window_pos.y + content_min.y + up_margin + up_offset);

  float popup_width = 250.f;
  float popup_height_padding = ImGui::GetStyle().ItemSpacing.y;
  float total_popup_height = m_autocomplete_candidates.size() * line_height +
                             popup_height_padding * 2.0f;
  float maximum_popup_height = 200.f;
  bool need_scroll = total_popup_height > maximum_popup_height;
  float popup_height = std::min(total_popup_height, maximum_popup_height);

  ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(popup_width, popup_height), ImGuiCond_Always);

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoFocusOnAppearing;

  if (!need_scroll) {
    flags |= ImGuiWindowFlags_NoScrollbar;
  }

  if (ImGui::Begin("##Autocomplete", nullptr, flags)) {
    for (size_t i = 0; i < m_autocomplete_candidates.size(); i++) {
      bool is_selected = (i == m_selected_candidate);

      if (is_selected) {
        ImGui::PushStyleColor(ImGuiCol_Header,
                              ImGui::GetColorU32(ImGuiCol_HeaderHovered));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                              ImGui::GetColorU32(ImGuiCol_HeaderHovered));
      }

      if (ImGui::Selectable(m_autocomplete_candidates[i].c_str(),
                            is_selected)) {
        insert_completion(m_autocomplete_candidates[i]);
        m_show_autocomplete_pooup = false;
      }

      if (is_selected) {
        ImGui::PopStyleColor(2);
        if (ImGui::IsItemVisible()) {
          ImGui::SetScrollHereY();
        }
      }
    }
    ImGui::End();
  }
}

bool basic_code_editor::mouse_scrolled_events_entry() {
  ImGuiIO &io = ImGui::GetIO();
  auto shift = io.KeyShift;
  auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;

  if (ctrl) {
    if (ImGui::IsKeyPressed(ImGuiKey_MouseWheelY)) {
      float delta = ImGui::GetIO().MouseWheel;
      if (delta > 0) {
        m_font_scale += 0.1f;
      } else {
        m_font_scale -= 0.1;
      }

      return true;
    }
  }

  return false;
}

void basic_code_editor::scan_for_context() {
  // TODO:@KeyFicller Scan for context.
}

void basic_code_editor::insert_completion(const std::string &_candidate) {
  auto cursor_pos = m_editor.GetCursorPosition();
  auto line = m_editor.GetCurrentLineText();

  TextEditor::UndoRecord u;

  u.mBefore = m_editor.mState;
  u.mRemovedEnd = m_editor.GetCursorPosition();

  auto cached_word = m_cached_seed.prefix;

  // Move cursor to word start
  TextEditor::Coordinates word_start_pos(
      cursor_pos.mLine, cursor_pos.mColumn - cached_word.length());
  m_editor.SetCursorPosition(word_start_pos);

  u.mRemoved = cached_word;
  u.mRemovedStart = word_start_pos;

  m_editor.SetUndoRecordOn(false);
  for (int i = 0; i < cached_word.length(); i++) {
    m_editor.Delete();
  }
  m_editor.SetUndoRecordOn(true);

  u.mAddedStart = m_editor.GetActualCursorCoordinates();
  m_editor.InsertText(_candidate);

  u.mAdded = _candidate;
  u.mAddedEnd = m_editor.GetActualCursorCoordinates();
  u.mAfter = m_editor.mState;
  m_editor.AddUndo(u);

  //  After insert completion, reset seed
  m_just_inserted_completion = true;
  m_cached_seed = {autocomplete_type::k_none, "", ""};
}

bool basic_code_editor::key_pressed_events_entry() {

  if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
    if (m_tab_to_indent) {
      TextEditor::UndoRecord u;
      u.mBefore = m_editor.mState;

      if (m_editor.HasSelection()) {
        u.mRemoved = m_editor.GetSelectedText();
        u.mRemovedStart = m_editor.mState.mSelectionStart;
        u.mRemovedEnd = m_editor.mState.mSelectionEnd;
        m_editor.DeleteSelection();
      }

      auto coord = m_editor.GetActualCursorCoordinates();
      u.mAddedStart = coord;

      auto line = m_editor.GetCurrentLineText();
      auto index = m_editor.GetCursorPosition().mColumn;
      auto left_index = index;
      while (left_index > 0 && line[left_index - 1] == ' ') {
        left_index--;
      }
      int space_count = index - left_index;
      space_count = 4 - (space_count % 4);
      std::string spaces(space_count, ' ');

      // TODO:@KeyFicller Handle overwrite case.
      // auto cindex = m_editor.GetCharacterIndex(coord);
      // if (m_editor.mOverwrite && cindex <
      // m_editor.mLines[coord.mLine].size()) {
      //   auto d = UTF8CharLength(m_editor.mLines[coord.mLine][cindex].mChar);
      //   u.mRemovedStart = m_editor.mState.mCursorPosition;
      //   u.mRemovedEnd = TextEditor::Coordinates(
      //       coord.mLine, m_editor.GetCharacterColumn(coord.mLine, cindex +
      //       d));
      //   while (d-- > 0 && cindex < m_editor.mLines[coord.mLine].size()) {
      //     u.mRemoved += m_editor.mLines[coord.mLine][cindex].mChar;
      //     m_editor.mLines[coord.mLine].erase(
      //         m_editor.mLines[coord.mLine].begin() + cindex);
      //   }
      // }

      m_editor.InsertText(spaces);

      u.mAdded = spaces;
      u.mAddedEnd = m_editor.GetActualCursorCoordinates();
      u.mAfter = m_editor.mState;
      m_editor.AddUndo(u);
      return true;
    }
  }

  if (key_pressed_events_for_save())
    return true;

  if (key_pressed_events_for_autocomplete())
    return true;

  return false;
}

bool basic_code_editor::key_pressed_events_for_save() {
  if (!m_save_callback)
    return false;

  ImGuiIO &io = ImGui::GetIO();
  auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
  if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
    return m_save_callback();
  }

  return false;
}

void basic_code_editor::set_save_callback(std::function<bool()> _callback) {
  m_save_callback = _callback;
}

bool basic_code_editor::key_pressed_events_for_autocomplete() {
  if (!m_show_autocomplete_pooup || m_autocomplete_candidates.empty()) {
    return false;
  }

  ImGuiIO &io = ImGui::GetIO();
  auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
  auto shift = io.KeyShift;
  auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

  if (ImGui::IsKeyPressed(ImGuiKey_Tab) ||
      ImGui::IsKeyPressed(ImGuiKey_Enter) ||
      ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
    if (!ctrl && !alt) {
      insert_completion(m_autocomplete_candidates[m_selected_candidate]);
      m_show_autocomplete_pooup = false;
      return true;
    }
  } else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && !ctrl && !alt) {
    m_selected_candidate =
        (m_selected_candidate - 1 + m_autocomplete_candidates.size()) %
        m_autocomplete_candidates.size();
    return true;
  } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && !ctrl && !alt) {
    m_selected_candidate =
        (m_selected_candidate + 1) % m_autocomplete_candidates.size();
    return true;
  } else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    m_show_autocomplete_pooup = false;
    return true;
  }

  return false;
}

const std::vector<std::string> &
basic_code_editor::get_builtin_keywords() const {
  static std::vector<std::string> keywords;
  return keywords;
}

const std::map<autocomplete_type, std::vector<std::string>> &
basic_code_editor::get_defined_keywords(const autocomplete_seed &_seed) const {
  static std::map<autocomplete_type, std::vector<std::string>> keywords;
  return keywords;
}

autocomplete_seed shader_editor::seed_for_autocomplete() const {
  int stop_index = m_editor.GetCursorPosition().mColumn;
  auto seed1 = seek_for_seed(m_editor.GetCurrentLineText(),
                             m_editor.GetCursorPosition().mColumn, &stop_index);
  auto line = m_editor.GetCurrentLineText();
  if (stop_index <= 0 || line[stop_index - 1] == ' ') {
    // Seed has no scope, it's a keyword.
    return autocomplete_seed{autocomplete_type::k_keyword, "", seed1};
  }

  if (stop_index > 0 && line[stop_index - 1] == '.') {
    auto scope = seek_for_seed(line, stop_index - 1);
    return {autocomplete_type::k_class_member, scope, seed1};
  }
  if (stop_index > 1 && line.substr(stop_index - 2, 2) == "->") {
    auto scope = seek_for_seed(line, stop_index - 2);
    return {autocomplete_type::k_class_member, scope, seed1};
  }
  if (stop_index > 1 && line.substr(stop_index - 2, 2) == "::") {
    auto scope = seek_for_seed(line, stop_index - 2);
    return {autocomplete_type::k_class_member, scope, seed1};
  }
  return {autocomplete_type::k_none, "", ""};
}

const std::vector<std::string> &shader_editor::get_builtin_keywords() const {
  static std::vector<std::string> glsl_keywords = {
      // Types
      "vec2", "vec3", "vec4", "mat2", "mat3", "mat4", "float", "int", "bool",
      "sampler2D", "samplerCube", "uniform", "in", "out", "inout", "const",
      // Control flow
      "if", "else", "for", "while", "return", "discard", "break", "continue",
      // Functions
      "texture", "texture2D", "textureCube", "mix", "clamp", "normalize",
      "length", "distance", "dot", "cross", "reflect", "refract", "sin", "cos",
      "tan", "asin", "acos", "atan", "atan2", "pow", "exp", "log", "exp2",
      "log2", "sqrt", "inversesqrt", "abs", "floor", "ceil", "fract", "mod",
      "min", "max", "step", "smoothstep", "smoothstep", "radians", "degrees",
      // Matrix functions
      "transpose", "determinant", "inverse",
      // Adavanced OpenGL internal variables
      "gl_Position",
      // TODO:@KeyFicller Add more advanced OpenGL internal variables.
  };
  return glsl_keywords;
}

const std::map<autocomplete_type, std::vector<std::string>> &
shader_editor::get_defined_keywords(const autocomplete_seed &_seed) const {
  return m_defined_keywords;
}

void shader_editor::format_text() {
  // TODO:@KeyFicller Format text
}

void shader_editor::scan_for_context() {
  m_defined_keywords.clear();
  if (!need_search_for_candidates()) {
    return;
  }

  scan_for_variables();
}

void shader_editor::scan_for_variables() {

  std::string text = m_editor.GetText();
  if (text.empty()) {
    return;
  }

  // GLSL built-in types
  static const std::vector<std::string> glsl_types = {"float",
                                                      "int",
                                                      "bool",
                                                      "vec2",
                                                      "vec3",
                                                      "vec4",
                                                      "mat2",
                                                      "mat3",
                                                      "mat4",
                                                      "sampler2D",
                                                      "samplerCube",
                                                      "sampler2DShadow",
                                                      "samplerCubeShadow",
                                                      "ivec2",
                                                      "ivec3",
                                                      "ivec4",
                                                      "bvec2",
                                                      "bvec3",
                                                      "bvec4",
                                                      "mat2x2",
                                                      "mat2x3",
                                                      "mat2x4",
                                                      "mat3x2",
                                                      "mat3x3",
                                                      "mat3x4",
                                                      "mat4x2",
                                                      "mat4x3",
                                                      "mat4x4"};

  static const std::map<std::string, std::vector<std::string>>
      glsl_type_members = {
          {"vec2", {"x", "y"}},
          {"vec3", {"x", "y", "z", "xy"}},
          {"vec4", {"x", "y", "z", "w", "xy", "xyz"}},
      };

  // Keywords that can appear before type
  static const std::vector<std::string> qualifiers = {
      "uniform", "in", "out", "inout", "const", "attribute", "varying"};

  std::vector<std::string> &keywords =
      m_defined_keywords[autocomplete_type::k_keyword];

  // Split text into lines
  std::vector<std::string> lines;
  std::string current_line;
  bool in_block_comment = false;

  for (size_t i = 0; i < text.length(); ++i) {
    char c = text[i];
    char next_c = (i + 1 < text.length()) ? text[i + 1] : '\0';

    // Handle block comments
    if (!in_block_comment && c == '/' && next_c == '*') {
      in_block_comment = true;
      ++i; // Skip next character
      continue;
    }
    if (in_block_comment && c == '*' && next_c == '/') {
      in_block_comment = false;
      ++i; // Skip next character
      continue;
    }
    if (in_block_comment) {
      continue;
    }

    // Handle line comments
    if (c == '/' && next_c == '/') {
      // Save current line (before comment) and skip rest of line
      if (!current_line.empty()) {
        lines.push_back(current_line);
        current_line.clear();
      }
      // Skip rest of line
      while (i < text.length() && text[i] != '\n' && text[i] != '\r') {
        ++i;
      }
      continue;
    }

    if (c == '\n' || (c == '\r' && next_c != '\n')) {
      if (!current_line.empty()) {
        lines.push_back(current_line);
        current_line.clear();
      }
    } else if (c != '\r') {
      // Skip \r if followed by \n (Windows line ending)
      current_line += c;
    }
  }
  if (!current_line.empty()) {
    lines.push_back(current_line);
  }

  // Parse each line for variable declarations
  for (const auto &line : lines) {
    if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
      continue;
    }

    // Remove leading/trailing whitespace
    std::string trimmed = line;
    size_t start = trimmed.find_first_not_of(" \t");
    if (start == std::string::npos) {
      continue;
    }
    trimmed = trimmed.substr(start);
    size_t end = trimmed.find_last_not_of(" \t");
    if (end != std::string::npos) {
      trimmed = trimmed.substr(0, end + 1);
    }

    // Skip preprocessor directives
    if (trimmed[0] == '#') {
      continue;
    }

    // Check for qualifiers
    std::string remaining = trimmed;
    bool found_qualifier = false;
    for (const auto &qualifier : qualifiers) {
      if (remaining.length() >= qualifier.length() + 1 &&
          remaining.substr(0, qualifier.length()) == qualifier &&
          (remaining[qualifier.length()] == ' ' ||
           remaining[qualifier.length()] == '\t')) {
        remaining = remaining.substr(qualifier.length());
        // Skip whitespace
        size_t ws_start = remaining.find_first_not_of(" \t");
        if (ws_start != std::string::npos) {
          remaining = remaining.substr(ws_start);
        }
        found_qualifier = true;
        break;
      }
    }

    // Check for layout qualifier
    if (remaining.length() >= 6 && remaining.substr(0, 6) == "layout") {
      size_t paren_start = remaining.find('(');
      if (paren_start != std::string::npos) {
        size_t paren_end = remaining.find(')', paren_start);
        if (paren_end != std::string::npos) {
          remaining = remaining.substr(paren_end + 1);
          // Skip whitespace
          size_t ws_start = remaining.find_first_not_of(" \t");
          if (ws_start != std::string::npos) {
            remaining = remaining.substr(ws_start);
          }
          // Check for in/out after layout
          if (remaining.length() >= 2 && remaining.substr(0, 2) == "in") {
            if (remaining.length() == 2 || remaining[2] == ' ' ||
                remaining[2] == '\t') {
              remaining = remaining.substr(2);
              size_t ws_start2 = remaining.find_first_not_of(" \t");
              if (ws_start2 != std::string::npos) {
                remaining = remaining.substr(ws_start2);
              }
            }
          } else if (remaining.length() >= 3 &&
                     remaining.substr(0, 3) == "out") {
            if (remaining.length() == 3 || remaining[3] == ' ' ||
                remaining[3] == '\t') {
              remaining = remaining.substr(3);
              size_t ws_start2 = remaining.find_first_not_of(" \t");
              if (ws_start2 != std::string::npos) {
                remaining = remaining.substr(ws_start2);
              }
            }
          }
        }
      }
    }

    // Find type
    std::string type;
    size_t type_end = 0;
    for (const auto &glsl_type : glsl_types) {
      if (remaining.length() >= glsl_type.length() &&
          remaining.substr(0, glsl_type.length()) == glsl_type) {
        if (remaining.length() == glsl_type.length() ||
            remaining[glsl_type.length()] == ' ' ||
            remaining[glsl_type.length()] == '\t') {
          type = glsl_type;
          type_end = glsl_type.length();
          break;
        }
      }
    }

    if (type.empty()) {
      continue;
    }

    // Skip whitespace after type
    remaining = remaining.substr(type_end);
    size_t ws_start = remaining.find_first_not_of(" \t");
    if (ws_start == std::string::npos) {
      continue;
    }
    remaining = remaining.substr(ws_start);

    // Extract variable name (may have array brackets)
    std::string var_name;
    for (size_t i = 0; i < remaining.length(); ++i) {
      char c = remaining[i];
      if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
        var_name += c;
      } else if (c == '[' || c == ';' || c == '=' || c == ' ' || c == '\t') {
        break;
      } else {
        // Invalid character, skip this line
        var_name.clear();
        break;
      }
    }

    if (!var_name.empty()) {
      if (m_cached_seed.type == autocomplete_type::k_keyword) {
        // Add to keywords if not already present
        bool exists = false;
        for (const auto &kw : keywords) {
          if (kw == var_name) {
            exists = true;
            break;
          }
        }
        if (!exists) {
          keywords.push_back(var_name);
        }
      } else if (m_cached_seed.type == autocomplete_type::k_class_member) {
        // Case1: if var_name is the same as scope, add members to key words.
        auto &members = m_defined_keywords[autocomplete_type::k_class_member];
        if (var_name == m_cached_seed.scope) {
          if (glsl_type_members.find(type) != glsl_type_members.end()) {
            for (const auto &member : glsl_type_members.at(type)) {
              if (member.length() > m_cached_seed.prefix.length() &&
                  member.substr(0, m_cached_seed.prefix.length()) ==
                      m_cached_seed.prefix) {
                members.push_back(member);
              }
            }
          }
        }
      }
    }
  }
}