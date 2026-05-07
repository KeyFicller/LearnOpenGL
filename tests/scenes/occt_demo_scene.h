#pragma once

#include "basic/light.h"
#include "basic/material.h"
#include "renderable_scene_base.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

// Open CASCADE demo: BRep box + sphere (fused), mesh, triangulation to GL.

class occt_demo_scene : public renderable_scene_base {
public:
  occt_demo_scene();
  ~occt_demo_scene() override = default;

  occt_demo_scene(const occt_demo_scene &) = delete;
  occt_demo_scene &operator=(const occt_demo_scene &) = delete;

  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;

private:
  void build_mesh_from_occt();

  std::vector<float> m_vertex_data;
  std::string m_status_message;
  glm::vec3 m_box_size{1.2f, 1.2f, 1.2f};
  glm::vec3 m_box_center{-1.5f, 0.0f, 0.0f};
  glm::vec3 m_sphere_center{1.2f, 0.0f, 0.0f};
  float m_sphere_radius = 0.65f;
  float m_mesh_deflection = 0.08f;
  size_t m_triangle_count = 0;

  light m_light{};
  material m_material{};
};
