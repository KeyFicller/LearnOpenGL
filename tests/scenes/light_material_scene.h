#pragma once

#include "renderable_scene_base.h"
#include "basic/light.h"
#include "basic/material.h"

class light_material_scene : public renderable_scene_base {
public:
  light_material_scene();
  virtual ~light_material_scene() = default;

public:
  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  light m_light;
  material m_material;
};