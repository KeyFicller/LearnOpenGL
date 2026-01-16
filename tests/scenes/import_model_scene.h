#pragma once

#include "renderable_scene_base.h"
#include "tests/scenes/import_model.h"

// Scene for testing model importing functionality
class import_model_scene : public renderable_scene_base {
public:
  import_model_scene();
  virtual ~import_model_scene() = default;

  // Disable copy constructor and copy assignment
  import_model_scene(const import_model_scene &) = delete;
  import_model_scene &operator=(const import_model_scene &) = delete;

  // Scene interface
  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  import_model m_import_model;
};