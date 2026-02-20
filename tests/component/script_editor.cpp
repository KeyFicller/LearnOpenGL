#include "script_editor.h"
#include "TextEditor.h"

const TextEditor::LanguageDefinition &
script_editor::script_language_defintion() {
  static TextEditor::LanguageDefinition definition =
      TextEditor::LanguageDefinition::CPlusPlus();
  definition.mName = "C#";
  return definition;
}

script_editor::script_editor(const std::string &_name)
    : basic_code_editor(_name, code_editor_type::k_script) {
  m_editor.SetLanguageDefinition(script_language_defintion());
}