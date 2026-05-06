#pragma once

#include "renderable_scene_base.h"
#include <string>
#include <vector>

// Open CASCADE demo: BRep sphere, incremental mesh, triangulation to GL buffers.

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
  float m_sphere_radius = 1.0f;
  float m_mesh_deflection = 0.08f;
  size_t m_triangle_count = 0;
};
