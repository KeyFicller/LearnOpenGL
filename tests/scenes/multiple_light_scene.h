#pragma once

#include "basic/light.h"
#include "basic/material.h"
#include "renderable_scene_base.h"

class multiple_light_scene : public renderable_scene_base {
public:
  multiple_light_scene();
  virtual ~multiple_light_scene() = default;

public:
  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  light m_lights[4];
  material m_material;
  int m_editing_light = 0;
};