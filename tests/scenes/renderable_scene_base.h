#pragma once

#include "basic/shader.h"
#include "camera_scene_base.h"
#include "tests/component/mesh_manager.h"
#include <glm/glm.hpp>

// Base class for scenes that render objects with model/view/projection matrices
class renderable_scene_base : public camera_scene_base {
public:
  renderable_scene_base(const std::string &_name);
  virtual ~renderable_scene_base();

protected:
  // Setup mesh with vertex and index data
  void setup_mesh(const mesh_data &data);

  // Set model/view/projection matrices for rendering
  void set_matrices(shader *_shader, const glm::mat4 &model = glm::mat4(1.0f));

  // Render mesh
  void render_mesh();

  // Render light source at position with scale
  void render_light_source(shader *_light_shader, const glm::vec3 &position,
                           const glm::vec3 &scale = glm::vec3(0.2f));

protected:
  mesh_manager m_mesh;
  shader *m_shader = nullptr;
  shader *m_light_shader = nullptr;
};
