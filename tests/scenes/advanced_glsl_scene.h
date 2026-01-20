#pragma once

#include "renderable_scene_base.h"
#include "tests/component/sub_scene.h"
#include "tests/scenes/advanced_glsl_sub_scenes.h"
#include <glm/glm.hpp>

class advanced_glsl_vertex_variable_sub_scene;
class advanced_glsl_fragment_variable_sub_scene;
class advanced_glsl_uniform_buffer_sub_scene;

class advanced_glsl_scene : public renderable_scene_base {
  friend class advanced_glsl_vertex_variable_sub_scene;
  friend class advanced_glsl_fragment_variable_sub_scene;
  friend class advanced_glsl_uniform_buffer_sub_scene;

public:
  advanced_glsl_scene();
  virtual ~advanced_glsl_scene() = default;

public:
  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

protected:
  // Sub-scenes need access to these members
  glm::vec3 m_light_position = glm::vec3{1.0f, 1.0f, 0.0f};

private:
  // Sub-scene manager
  sub_scene_manager<advanced_glsl_scene> m_sub_scene_manager;
};