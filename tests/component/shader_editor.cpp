#include "shader_editor.h"
#include "imgui.h"

namespace {
std::string seek_for_seed(const std::string &_current_line, int _index) {
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
    return _current_line.substr(seed_start, _index - seed_start);
  }

  return "";
}
}; // namespace

basic_code_editor::basic_code_editor(const std::string &_name,
                                     code_editor_type _type)
    : m_name(_name) {
  switch (_type) {
  case code_editor_type::k_shader:
    m_editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());
    break;
  default:
    break;
  }

  m_editor.SetKeyPressedCallback(
      std::bind(&basic_code_editor::key_pressed_events_entry, this));
}

void basic_code_editor::set_text(const std::string &_text) {
  m_editor.SetText(_text);
}

std::string basic_code_editor::get_text() const { return m_editor.GetText(); }

void basic_code_editor::set_help_info(const std::string &_hint) {
  m_help_info = _hint;
}

std::pair<autocomplete_type, std::string>
basic_code_editor::seed_for_autocomplete() const {
  return {autocomplete_type::k_keyword,
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

  m_editor.Render((m_name + "Impl").c_str());

  if (m_editor.IsTextChanged())
    update_candidates();

  if (m_show_autocomplete_pooup && !m_autocomplete_candidates.empty())
    show_autocomplete_popup();

  if (m_editor.IsTextChanged())
    restore_default_help_info();

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
  if (seed.second != m_cached_word) {
    m_cached_word = seed.second;
    if (m_cached_word.empty()) {
      m_show_autocomplete_pooup = false;
      m_autocomplete_candidates.clear();
      return;
    }
  } else {
    return;
  }

  m_autocomplete_candidates.clear();

  if (seed.first == autocomplete_type::k_keyword) {
    auto &builtin_keywords = get_builtin_keywords();
    for (const auto &keyword : builtin_keywords) {
      if (keyword.length() >= seed.second.length() &&
          keyword.substr(0, seed.second.length()) == seed.second) {
        m_autocomplete_candidates.push_back(keyword);
      }
    }
  }
  auto &defined_keywords = get_defined_keywords();
  if (defined_keywords.find(seed.first) != defined_keywords.end()) {
    for (const auto &keyword : defined_keywords.at(seed.first)) {
      if (keyword.length() >= seed.second.length() &&
          keyword.substr(0, seed.second.length()) == seed.second) {
        m_autocomplete_candidates.push_back(keyword);
      }
    }
  }

  std::sort(m_autocomplete_candidates.begin(), m_autocomplete_candidates.end());
  m_selected_candidate = 0;

  m_show_autocomplete_pooup =
      !m_autocomplete_candidates.empty() && seed.second.length() > 0;
  if (m_autocomplete_candidates.size() == 1 &&
      m_autocomplete_candidates[0] == seed.second) {
    m_show_autocomplete_pooup = false;
  }
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

void basic_code_editor::insert_completion(const std::string &_candidate) {
  // TODO:@KeyFicller Combine into one Undo/Redo operation.
  auto cursor_pos = m_editor.GetCursorPosition();
  auto line = m_editor.GetCurrentLineText();

  // Move cursor to word start
  TextEditor::Coordinates word_start_pos(
      cursor_pos.mLine, cursor_pos.mColumn - m_cached_word.length());
  m_editor.SetCursorPosition(word_start_pos);

  for (int i = 0; i < m_cached_word.length(); i++) {
    m_editor.Delete();
  }

  m_editor.InsertText(_candidate);
}

bool basic_code_editor::key_pressed_events_entry() {

  if (ImGui::IsKeyPressed(ImGuiKey_Tab)) {
    if (m_tab_to_indent) {
      auto line = m_editor.GetCurrentLineText();
      auto index = m_editor.GetCursorPosition().mColumn;
      auto left_index = index;
      while (left_index > 0 && line[left_index - 1] == ' ') {
        left_index--;
      }
      int space_count = index - left_index;
      space_count = 4 - (space_count % 4);
      std::string spaces(space_count, ' ');
      m_editor.InsertText(spaces);
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
basic_code_editor::get_defined_keywords() const {
  static std::map<autocomplete_type, std::vector<std::string>> keywords;
  return keywords;
}

std::pair<autocomplete_type, std::string>
shader_editor::seed_for_autocomplete() const {
  auto seed = basic_code_editor::seed_for_autocomplete();
  if (!seed.second.empty()) {
    return seed;
  }

  auto line = m_editor.GetCurrentLineText();
  auto index = m_editor.GetCursorPosition().mColumn;
  if (index - 1 > 0 && line[index - 1] == '.') {
    return {autocomplete_type::k_class_member, seek_for_seed(line, index - 1)};
  }
  if (index - 2 > 0 && line.substr(index - 2, 2) == "->") {
    return {autocomplete_type::k_class_member, seek_for_seed(line, index - 2)};
  }
  if (index - 2 > 0 && line.substr(index - 2, 2) == "::") {
    return {autocomplete_type::k_class_member, seek_for_seed(line, index - 2)};
  }
  return {autocomplete_type::k_none, ""};
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
      "transpose", "determinant", "inverse"};
  return glsl_keywords;
}

const std::map<autocomplete_type, std::vector<std::string>> &
shader_editor::get_defined_keywords() const {
  return m_defined_keywords;
}

void shader_editor::format_text() {
  // TODO:@KeyFicller Format text
}