#pragma once

#include "basic/shader.h"
#include "tests/cad/database/database.h"
#include "tests/cad/history/history_tree.h"
#include "tests/cad/renderer/disp_var.h"
#include "tests/cad/renderer/viewport_axes_gizmo.h"

#include <memory>
#include <vector>

struct GLFWwindow;

namespace toy_cad::interaction {

class input_handler;
class debug_panel;

}

namespace toy_cad {

class instance {
public:
  static instance &get() {
    static instance inst;
    return inst;
  }

  instance(const instance &) = delete;
  instance &operator=(const instance &) = delete;
  instance(instance &&) = delete;
  instance &operator=(instance &&) = delete;

  void init(GLFWwindow *window);
  void update(float delta_time);
  void render();
  void render_ui();

  bool on_mouse_moved(double xpos, double ypos);
  bool on_mouse_scroll(double xoffset, double yoffset);
  bool on_mouse_button(int button, int action, int mods);
  void on_framebuffer_resized(int width, int height);
  void on_object_hovered(int object_id);

  /** Non-owning pointers; singleton `doc_input_handler` is pushed in `init`. */
  void push_input_handler(interaction::input_handler *handler);
  void pop_input_handler();
  interaction::input_handler *top_input_handler();

  [[nodiscard]] database &db() { return m_database; }
  [[nodiscard]] const database &db() const { return m_database; }

  [[nodiscard]] disp_var &disp() { return m_disp; }
  [[nodiscard]] const disp_var &disp() const { return m_disp; }

  [[nodiscard]] history_tree &history() { return m_history; }
  [[nodiscard]] const history_tree &history() const { return m_history; }

  [[nodiscard]] GLFWwindow *window() const { return m_window; }

  [[nodiscard]] shader &viewport_shader() { return *m_vp_shader; }

  [[nodiscard]] shader &global_shader() { return *m_global_shader; }

  [[nodiscard]] viewport_axes_gizmo &viewport_axes() { return m_viewport_axes; }
  [[nodiscard]] const viewport_axes_gizmo &viewport_axes() const {
    return m_viewport_axes;
  }

private:
  instance() = default;

  GLFWwindow *m_window = nullptr;
  database m_database{};
  disp_var m_disp{};
  history_tree m_history{};
  std::vector<interaction::input_handler *> m_input_stack{};

  std::unique_ptr<shader> m_vp_shader{};
  std::unique_ptr<shader> m_global_shader{};
  viewport_axes_gizmo m_viewport_axes{};
};

} // namespace toy_cad
