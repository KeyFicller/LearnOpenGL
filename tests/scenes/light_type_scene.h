#pragma once

#include "basic/light.h"
#include "basic/material.h"
#include "renderable_scene_base.h"

class light_type_scene : public renderable_scene_base {
public:
  light_type_scene();
  virtual ~light_type_scene() = default;

public:
  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  light m_light;
  material m_material;
};