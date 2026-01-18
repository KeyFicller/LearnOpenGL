#pragma once

#include "renderable_scene_base.h"
#include "tests/component/sub_scene.h"
#include <glm/glm.hpp>

// Forward declarations
class stencil_object_outline_sub_scene;
class stencil_mirror_sub_scene;

class stencil_test_scene : public renderable_scene_base {
  // Friend declarations - allow sub-scenes to access protected members
  friend class stencil_object_outline_sub_scene;
  friend class stencil_mirror_sub_scene;

public:
  stencil_test_scene();
  virtual ~stencil_test_scene() = default;

public:
  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

protected:
  // Sub-scenes need access to these members
  // Lighting parameters (shared)
  glm::vec3 m_light_color = glm::vec3{1.0f, 1.0f, 1.0f};
  glm::vec3 m_object_color = glm::vec3{0.8f, 0.2f, 0.0f};
  glm::vec3 m_light_position = glm::vec3{1.0f, 1.0f, 0.0f};
  float m_ambient_strength = 0.1f;
  float m_specular_strength = 0.5f;
  float m_shininess = 32.0f;

private:
  // Sub-scene manager
  sub_scene_manager<stencil_test_scene> m_sub_scene_manager;
};