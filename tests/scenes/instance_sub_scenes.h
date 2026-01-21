#pragma once

#include "basic/shader.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/sub_scene.h"
#include "tests/scenes/instance_scene.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

// Forward declaration
class instance_scene;

/**
 * @brief Sub-scene: Instance - Uniform Pass Value
 */
class instance_uniform_pass_value_scene : public sub_scene<instance_scene> {
public:
  instance_uniform_pass_value_scene(instance_scene *_parent);
  ~instance_uniform_pass_value_scene() override;

  void render() override;
  void render_ui() override {}

private:
  shader *m_shader = nullptr;
};

class instance_attrib_pass_value_scene : public sub_scene<instance_scene> {
public:
  instance_attrib_pass_value_scene(instance_scene *_parent);
  ~instance_attrib_pass_value_scene() override;

  void render() override;
  void render_ui() override {}

private:
  shader *m_shader = nullptr;
  unsigned int m_instanceVBO;
  glm::vec2 m_translations[100];
};