#pragma once

#include "shader_editor.h"

class script_editor : public basic_code_editor {
  static const TextEditor::LanguageDefinition &script_language_defintion();

public:
  script_editor(const std::string &_name);
  virtual ~script_editor() = default;
};