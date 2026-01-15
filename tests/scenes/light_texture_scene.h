#pragma once

#include "renderable_scene_base.h"
#include "basic/light.h"
#include "basic/material.h"

class light_texture_scene : public renderable_scene_base {
public:
  light_texture_scene();
  virtual ~light_texture_scene() = default;

public:
  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  light m_light;
  texture_material m_texture_material;
};