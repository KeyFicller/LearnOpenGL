#pragma once

#include "basic/shader.h"
#include "renderable_scene_base.h"
#include "soft_body_dirver.h"
#include "tests/component/mesh_manager.h"
#include <glm/glm.hpp>
#include <vector>

class soft_body_frog_scene : public renderable_scene_base {
public:
  soft_body_frog_scene();
  ~soft_body_frog_scene() override;

  void init(GLFWwindow *_window) override;
  void render() override;
  void render_ui() override;
  void update(float _delta_time) override;

private:
  void init_frog_loop();
  std::vector<int> get_loop_point_order(size_t _loop_index) const;
  void update_mesh_data();

  soft_body_dirver m_driver;
  mesh_manager m_body_mesh;
  mesh_manager m_outline_mesh;
  mesh_manager m_eye_mesh;
  mesh_manager m_mouth_mesh;

  shader *m_body_shader = nullptr;
  shader *m_outline_shader = nullptr;
  shader *m_eye_shader = nullptr;
  shader *m_mouth_shader = nullptr;

  glm::vec3 m_body_color = glm::vec3(0.427f, 0.682f, 0.624f);
  glm::vec3 m_eye_ring_color = glm::vec3(0.831f, 0.525f, 0.431f);
  glm::vec3 m_outline_color = glm::vec3(0.0f, 0.0f, 0.0f);
  float m_outline_width = 0.008f;
  float m_eye_outer_radius = 0.032f;
  float m_eye_teal_radius = 0.026f;
  float m_eye_inner_radius = 0.018f;
  float m_eye_pupil_radius = 0.008f;
  float m_eye_offset_y = 0.055f;
  float m_eye_offset_x = 0.042f;
  float m_mouth_scale = 0.045f;
};
