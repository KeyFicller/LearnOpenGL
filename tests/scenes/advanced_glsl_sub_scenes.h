#pragma once

#include "basic/shader.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/sub_scene.h"
#include <glm/glm.hpp>
#include <memory>

// Forward declaration
class advanced_glsl_scene;

/**
 * @brief Sub-scene: Advanced GLSL glPointSize test
 */
class advanced_glsl_vertex_variable_sub_scene
    : public sub_scene<advanced_glsl_scene> {
public:
  advanced_glsl_vertex_variable_sub_scene(advanced_glsl_scene *parent);

  void render() override;
  void render_ui() override;

private:
  float m_point_size = 1.0f;
  bool m_increment_point_size = true;
  mesh_manager m_point_mesh;
};

class advanced_glsl_fragment_variable_sub_scene
    : public sub_scene<advanced_glsl_scene> {
public:
  advanced_glsl_fragment_variable_sub_scene(advanced_glsl_scene *parent);

  void render() override;
  void render_ui() override;

private:
  bool m_split_screen = false;
  float m_split_base = 400.0f;
  bool m_front_facing = false;
};

class advanced_glsl_uniform_buffer_sub_scene
    : public sub_scene<advanced_glsl_scene> {
public:
  advanced_glsl_uniform_buffer_sub_scene(advanced_glsl_scene *parent);

  void render() override;
  void render_ui() override;

private:
  struct object_info {
    shader *Shader = nullptr;
    glm::vec3 Position;
    glm::vec3 ObjectColor;

    ~object_info() { delete Shader; }
  };

  std::vector<std::unique_ptr<object_info>> m_objects;
  unsigned int m_ubo_handle = -1;
};
