#include "shader_editor_scene.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include <algorithm>
#include <cctype>

static float quad_vertices[] = {-1.0f, -1.0f, 0.0f, 1.0f,  -1.0f, 0.0f,
                                1.0f,  1.0f,  0.0f, -1.0f, 1.0f,  0.0f};

static unsigned int quad_indices[] = {0, 1, 2, 2, 3, 0};

shader_editor_scene::shader_editor_scene() : test_scene_base("Shader Editor") {}

shader_editor_scene::~shader_editor_scene() {}

void shader_editor_scene::init(GLFWwindow *_window) {
  test_scene_base::init(_window);

  // Set up editor with GLSL language definition
  m_editor.SetLanguageDefinition(TextEditor::LanguageDefinition::GLSL());

  m_editor.SetKeyPressedCallback(
      std::bind(&shader_editor_scene::on_key_pressed_with_popup, this));

  mesh_data data(quad_vertices, sizeof(quad_vertices), quad_indices,
                 sizeof(quad_indices) / sizeof(unsigned int),
                 {{3, GL_FLOAT, GL_FALSE}});
  m_mesh_manager.setup_mesh(data);
}

void shader_editor_scene::render() {
  if (!m_shader) {
    return;
  }
  m_shader->use();
  m_mesh_manager.draw();
}

void shader_editor_scene::render_ui() {

  if (!m_is_editing_fragment_shader) {
    if (ImGui::Button("Edit Fragment Shader")) {
      m_is_editing_fragment_shader = true;
      m_editor.SetText(m_fragment_shader_source);
    }
  } else {
    auto cpos = m_editor.GetCursorPosition();
    ImGui::Begin("Fragment Shader Editor");
    ImGui::TextDisabled("%s", m_edit_hint.c_str());

    ImGui::Text(
        "%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1,
        cpos.mColumn + 1, m_editor.GetTotalLines(),
        m_editor.IsOverwrite() ? "Ovr" : "Ins", m_editor.CanUndo() ? "*" : " ",
        m_editor.GetLanguageDefinition().mName.c_str(), "Fragment Shader");

    m_editor.Render("Test");

    // Update autocomplete only when current word changes
    std::string new_current_word = get_current_word();
    if (m_editor.IsTextChanged() && new_current_word != m_current_word) {
      m_current_word = new_current_word;
      if (m_current_word.length() > 0) {
        update_autocomplete(m_current_word);
      } else {
        m_show_autocomplete = false;
      }
    }

    // Show autocomplete popup
    // Keyboard input is now handled in on_key_pressed_with_popup callback
    if (m_show_autocomplete && !m_completions.empty()) {
      show_autocomplete_popup();
    }

    if (m_editor.IsTextChanged()) {
      // Restore Hint
      m_edit_hint = "Ctrl+S to save";
    }

    // Handle Ctrl+S to save
    ImGuiIO &io = ImGui::GetIO();
    bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
      m_fragment_shader_source = m_editor.GetText();
      std::string error_message;
      if (!compile_and_replace_shader(&error_message)) {
        m_edit_hint = error_message;
      } else {
        m_is_editing_fragment_shader = false;
      }
    }

    ImGui::End();
  }

  if (ImGui::Button("Compile and Replace Shader")) {
    if (!compile_and_replace_shader()) {
      ImGui::BeginPopupModal("Compile Error", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize);
      ImGui::Text("Failed to compile shader");
      ImGui::EndPopup();
    }
  }
}

bool shader_editor_scene::compile_and_replace_shader(
    std::string *_error_message) {
  try {
    shader *new_shader = shader::shader_from_source(m_vertex_shader_source,
                                                    m_fragment_shader_source);
    if (new_shader) {
      if (m_shader) {
        delete m_shader;
      }
      m_shader = new_shader;
      return true;
    } else {
      return false;
    }
  } catch (std::exception &e) {
    if (_error_message) {
      *_error_message = e.what();
    }
    return false;
  }
}

std::string shader_editor_scene::get_current_word() {
  auto cpos = m_editor.GetCursorPosition();
  std::vector<std::string> lines = m_editor.GetTextLines();

  if (cpos.mLine < 0 || cpos.mLine >= static_cast<int>(lines.size())) {
    return "";
  }

  std::string current_line = lines[cpos.mLine];
  if (current_line.empty() || cpos.mColumn == 0) {
    return "";
  }

  // Clamp column to line length
  int column = std::min(cpos.mColumn, static_cast<int>(current_line.length()));

  // Find word start (alphanumeric and underscore)
  int word_start = column;
  while (
      word_start > 0 && word_start <= static_cast<int>(current_line.length()) &&
      (std::isalnum(static_cast<unsigned char>(current_line[word_start - 1])) ||
       current_line[word_start - 1] == '_')) {
    word_start--;
  }

  // Extract word
  if (word_start < static_cast<int>(current_line.length()) &&
      word_start < column) {
    return current_line.substr(word_start, column - word_start);
  }

  return "";
}

void shader_editor_scene::update_autocomplete(const std::string &prefix) {
  // Only update if prefix actually changed
  static std::string last_prefix;
  if (prefix == last_prefix && !m_completions.empty()) {
    return; // Prefix unchanged, keep existing completions and selection
  }
  last_prefix = prefix;

  m_completions.clear();

  // GLSL keywords and functions
  std::vector<std::string> glsl_keywords = {
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

  // Filter matching completions
  for (const auto &keyword : glsl_keywords) {
    if (keyword.length() >= prefix.length() &&
        keyword.substr(0, prefix.length()) == prefix) {
      m_completions.push_back(keyword);
    }
  }

  // Sort completions
  std::sort(m_completions.begin(), m_completions.end());

  // Only reset selection if completions changed
  m_selected_completion = 0;

  // Don't show popup if there's only one match and it's exactly the same as the
  // prefix
  bool should_show = !m_completions.empty() && prefix.length() > 0;
  if (should_show && m_completions.size() == 1 && m_completions[0] == prefix) {
    should_show = false;
  }
  m_show_autocomplete = should_show;
}

void shader_editor_scene::show_autocomplete_popup() {
  if (m_completions.empty())
    return;

  // Get the editor window position
  ImVec2 window_pos = ImGui::GetWindowPos();
  ImVec2 content_min = ImGui::GetWindowContentRegionMin();

  // Get cursor position in the editor
  auto cpos = m_editor.GetCursorPosition();

  // Calculate popup position (below the cursor, aligned to cursor column)
  float line_height = ImGui::GetTextLineHeight();

  // Calculate line number area width based on total lines
  // TextEditor uses mTextStart which is calculated as: max line number width +
  // mLeftMargin (10)
  int total_lines = m_editor.GetTotalLines();
  std::string max_line_number = std::to_string(total_lines);
  float line_number_width = ImGui::CalcTextSize(max_line_number.c_str()).x;
  float left_margin = ImGui::CalcTextSize("0000").x; // TextEditor's mLeftMargin
  float text_start = line_number_width + left_margin; // This is mTextStart

  // Get the current line text to calculate actual cursor position
  std::vector<std::string> lines = m_editor.GetTextLines();
  float cursor_x_offset = 0.0f;
  if (cpos.mLine >= 0 && cpos.mLine < static_cast<int>(lines.size())) {
    // Calculate the width of text before cursor
    std::string line_before_cursor = lines[cpos.mLine].substr(0, cpos.mColumn);
    cursor_x_offset = ImGui::CalcTextSize(line_before_cursor.c_str()).x;
  }

  // Position popup below the cursor, aligned to cursor column
  // Add text_start to account for line number area
  ImVec2 popup_pos = ImVec2(
      window_pos.x + content_min.x + text_start + cursor_x_offset,
      window_pos.y + content_min.y + (cpos.mLine + 2) * line_height + 10.0f);

  // Calculate popup size to fit all items without scrolling
  float item_height = line_height;
  float padding = ImGui::GetStyle().WindowPadding.y * 2.0f;
  float popup_height =
      static_cast<float>(m_completions.size()) * item_height + padding;
  float max_height = 200.0f;

  // If content is too tall, limit height and enable scrolling
  bool needs_scrolling = popup_height > max_height;
  if (needs_scrolling) {
    popup_height = max_height;
  }

  float popup_width = 250.0f;

  ImGui::SetNextWindowPos(popup_pos, ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(popup_width, popup_height), ImGuiCond_Always);

  // Use Popup flags to allow interaction while keeping editor focus
  // Only disable scrollbar if content fits without scrolling
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoFocusOnAppearing;

  if (!needs_scrolling) {
    flags |= ImGuiWindowFlags_NoScrollbar;
  }

  if (ImGui::Begin("##Autocomplete", nullptr, flags)) {
    for (size_t i = 0; i < m_completions.size(); i++) {
      bool is_selected = (i == m_selected_completion);

      if (is_selected) {
        ImGui::PushStyleColor(ImGuiCol_Header,
                              ImGui::GetColorU32(ImGuiCol_HeaderHovered));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                              ImGui::GetColorU32(ImGuiCol_HeaderHovered));
      }

      if (ImGui::Selectable(m_completions[i].c_str(), is_selected)) {
        insert_completion(m_completions[i]);
        m_show_autocomplete = false;
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

void shader_editor_scene::insert_completion(const std::string &completion) {
  // Get current cursor position
  auto cpos = m_editor.GetCursorPosition();
  std::vector<std::string> lines = m_editor.GetTextLines();

  if (cpos.mLine < 0 || cpos.mLine >= static_cast<int>(lines.size())) {
    // Just insert the completion
    m_editor.InsertText(completion);
    return;
  }

  std::string current_line = lines[cpos.mLine];
  if (current_line.empty() || cpos.mColumn == 0) {
    // Just insert the completion
    m_editor.InsertText(completion);
    return;
  }

  // Clamp column to line length
  int column = std::min(cpos.mColumn, static_cast<int>(current_line.length()));

  // Find word start (alphanumeric and underscore)
  int word_start = column;
  while (
      word_start > 0 && word_start <= static_cast<int>(current_line.length()) &&
      (std::isalnum(static_cast<unsigned char>(current_line[word_start - 1])) ||
       current_line[word_start - 1] == '_')) {
    word_start--;
  }

  // If we found a word to replace
  if (word_start < column) {
    // Move cursor to word start
    TextEditor::Coordinates word_start_pos(cpos.mLine, word_start);
    m_editor.SetCursorPosition(word_start_pos);

    // Delete characters from word_start to original cursor position
    int chars_to_delete = column - word_start;
    for (int i = 0; i < chars_to_delete; i++) {
      m_editor.Delete(); // Delete() deletes the character at cursor position
    }
  }

  // Insert completion
  m_editor.InsertText(completion);
}

bool shader_editor_scene::on_key_pressed_with_popup() {
  // Only handle autocomplete if popup is showing
  if (!m_show_autocomplete || m_completions.empty()) {
    return false;
  }

  ImGuiIO &io = ImGui::GetIO();
  auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
  auto shift = io.KeyShift;
  auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

  // Handle autocomplete navigation and selection
  // Tab or Enter: Insert selected completion
  if (ImGui::IsKeyPressed(ImGuiKey_Tab) ||
      ImGui::IsKeyPressed(ImGuiKey_Enter) ||
      ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
    // Only handle if no modifiers are pressed (except Shift for Tab)
    if (!ctrl && !alt) {
      insert_completion(m_completions[m_selected_completion]);
      m_show_autocomplete = false;
      return true; // Consume the key, prevent TextEditor from handling it
    }
  }
  // Up/Down arrows: Navigate completion list
  else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && !ctrl && !alt) {
    m_selected_completion = (m_selected_completion - 1 + m_completions.size()) %
                            m_completions.size();
    return true; // Consume the key, prevent TextEditor from moving cursor
  } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && !ctrl && !alt) {
    m_selected_completion = (m_selected_completion + 1) % m_completions.size();
    return true; // Consume the key, prevent TextEditor from moving cursor
  }
  // Escape: Close autocomplete popup
  else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
    m_show_autocomplete = false;
    return true; // Consume the key
  }

  // If we reach here, we didn't handle the key
  // Let TextEditor handle it normally
  return false;
}