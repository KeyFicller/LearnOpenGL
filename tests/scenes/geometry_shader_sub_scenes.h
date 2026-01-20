#pragma once

#include "basic/shader.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/sub_scene.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

// Forward declaration
class geometry_shader_scene;

/**
 * @brief Sub-scene: Geometry Shader - Line Strip
 */
class geometry_shader_sub_scene : public sub_scene<geometry_shader_scene> {
public:
  geometry_shader_sub_scene(geometry_shader_scene *_parent,
                            const std::string &_shader_type,
                            const std::string &_geometry_shader);
  ~geometry_shader_sub_scene() override;

  void render() override;
  void render_ui() override;

private:
  shader *m_shader = nullptr;
  float m_shape_factor = 1.0f;
};

class geometry_shader_line_strip_scene : public geometry_shader_sub_scene {
public:
  geometry_shader_line_strip_scene(geometry_shader_scene *_parent)
      : geometry_shader_sub_scene(
            _parent, "Line Strip",
            "shaders/geometry_shader_test/line_strip.shader") {}
};

class geometry_shader_triangle_strip_scene : public geometry_shader_sub_scene {
public:
  geometry_shader_triangle_strip_scene(geometry_shader_scene *_parent)
      : geometry_shader_sub_scene(
            _parent, "Triangle Strip",
            "shaders/geometry_shader_test/triangle_strip.shader") {}
};
