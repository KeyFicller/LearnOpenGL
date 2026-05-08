#include "basic/imgui_font_setup.h"

#include "tests/component/shader_editor.h"

#include <GLFW/glfw3.h>

#include <algorithm>

void setup_imgui_fonts(ImGuiIO &io, GLFWwindow *window) {
  io.Fonts->AddFontDefault();

  ImFontConfig font_cfg{};
  font_cfg.MergeMode = true;
  io.Fonts->AddFontFromFileTTF("assets/fonts/Arial Unicode.ttf", 18.0f,
                               &font_cfg,
                               io.Fonts->GetGlyphRangesChineseFull());

  float content_scale_x = 1.0f;
  float content_scale_y = 1.0f;
  if (window) {
    glfwGetWindowContentScale(window, &content_scale_x, &content_scale_y);
  }
  const float content_scale = std::max(content_scale_x, content_scale_y);

  ImFontConfig code_cfg{};
  code_cfg.OversampleH = 3;
  code_cfg.OversampleV = 3;
  code_cfg.PixelSnapH = false;
  code_cfg.RasterizerDensity =
      (content_scale > 1.01f) ? content_scale : 1.0f;

  ImFont *code_editor_font = io.Fonts->AddFontFromFileTTF(
      "assets/fonts/Arial Unicode.ttf", 20.0f, &code_cfg,
      io.Fonts->GetGlyphRangesChineseFull());
  if (!code_editor_font) {
    code_editor_font = io.Fonts->Fonts[0];
  }
  register_code_editor_imgui_font(code_editor_font);
}
