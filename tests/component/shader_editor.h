#pragma once

#include "TextEditor.h"
#include <string>

enum class code_editor_type { k_shader, k_script };
enum class autocomplete_type { k_none, k_keyword, k_class_member };

struct autocomplete_seed {
  autocomplete_type type;
  std::string scope;
  std::string prefix;

  autocomplete_seed()
      : type(autocomplete_type::k_none), scope(""), prefix("") {}

  autocomplete_seed(autocomplete_type _type, const std::string &_scope,
                    const std::string &_prefix)
      : type(_type), scope(_scope), prefix(_prefix) {}

  bool operator==(const autocomplete_seed &_other) const {
    return type == _other.type && scope == _other.scope &&
           prefix == _other.prefix;
  }

  bool operator!=(const autocomplete_seed &_other) const {
    return !(*this == _other);
  }
};

class basic_code_editor {
  // friend class shader_editor_scene;

public:
  basic_code_editor(const std::string &_name, code_editor_type _type);
  virtual ~basic_code_editor() = default;

public:
  void set_text(const std::string &_text);
  std::string get_text() const;
  void render();
  void set_help_info(const std::string &_hint);
  void set_save_callback(std::function<bool()> _callback);
  void restore_default_help_info();
  void set_tab_to_indent(bool _tab_to_indent);
  virtual void format_text();

protected:
  void update_candidates();
  void show_autocomplete_popup();
  void insert_completion(const std::string &_candidate);
  bool key_pressed_events_entry();
  bool key_pressed_events_for_save();
  bool key_pressed_events_for_autocomplete();
  bool mouse_scrolled_events_entry();
  void draw_help_info();
  virtual void scan_for_context();
  bool need_search_for_candidates() const;

  // virtual methods for candidates searching
  virtual autocomplete_seed seed_for_autocomplete() const;
  virtual const std::vector<std::string> &get_builtin_keywords() const;
  virtual const std::map<autocomplete_type, std::vector<std::string>> &
  get_defined_keywords(const autocomplete_seed &_seed) const;

protected:
  std::string m_name;
  TextEditor m_editor;

  std::string m_help_info;
  std::vector<std::string> m_autocomplete_candidates;
  int m_selected_candidate;
  bool m_show_autocomplete_pooup = false;
  autocomplete_seed m_cached_seed;

  std::function<bool()> m_save_callback = nullptr;
  bool m_tab_to_indent = true;
  float m_font_scale = 2.0f;

  bool m_just_inserted_completion = false;
};

class shader_editor : public basic_code_editor {
public:
  shader_editor(const std::string &_name)
      : basic_code_editor(_name, code_editor_type::k_shader) {}
  virtual ~shader_editor() = default;

protected:
  autocomplete_seed seed_for_autocomplete() const override;
  const std::vector<std::string> &get_builtin_keywords() const override;
  const std::map<autocomplete_type, std::vector<std::string>> &
  get_defined_keywords(const autocomplete_seed &_seed) const override;
  void format_text() override;

private:
  void scan_for_context() override;
  void scan_for_variables();

protected:
  std::map<autocomplete_type, std::vector<std::string>> m_defined_keywords;
};

class script_editor : public basic_code_editor {
public:
  script_editor(const std::string &_name)
      : basic_code_editor(_name, code_editor_type::k_script) {}
  virtual ~script_editor() = default;
};