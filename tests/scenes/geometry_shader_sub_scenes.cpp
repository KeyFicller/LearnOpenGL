#include "geometry_shader_sub_scenes.h"
#include "geometry_shader_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_cube.h"
#include <glm/gtc/matrix_transform.hpp>

// ============================================================================
// geometry_shader_line_strip_sub_scene
// ============================================================================

geometry_shader_point_sub_scene::geometry_shader_point_sub_scene(
    geometry_shader_scene *_parent, const std::string &_shader_type,
    const std::string &_geometry_shader)
    : sub_scene<geometry_shader_scene>(_parent, _shader_type) {

  // Setup mesh
  std::vector<glm::vec3> points = {
      {-0.5f, -0.5f, 0.0f},
      {1.0f, 0.0f, 0.0f},
      {0.5f, 0.5f, 0.0f},
      {0.5f, -0.5f, 0.0f},
  };
  mesh_data points_mesh_data(points.data(), points.size() * sizeof(glm::vec3),
                             points.size(),
                             {vertex_attribute{3, GL_FLOAT, false}});
  m_points_mesh_manager.setup_mesh(points_mesh_data);

  m_shader = new shader("shaders/geometry_shader_test/vertex_for_points.shader",
                        "shaders/geometry_shader_test/fragment.shader",
                        _geometry_shader.c_str());
}

geometry_shader_point_sub_scene::~geometry_shader_point_sub_scene() {
  delete m_shader;
}

void geometry_shader_point_sub_scene::render() {
  if (!m_shader) {
    return;
  }

  m_parent->set_matrices(m_shader);
  m_shader->use();
  m_points_mesh_manager.bind();
  m_shader->set_uniform("uShapeFactor", m_shape_factor);
  glDrawArrays(GL_POINTS, 0, m_points_mesh_manager.get_index_count());
}

void geometry_shader_point_sub_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Geometry Shader - %s", name().c_str());
  ImGui::Spacing();

  ImGui::SliderFloat("Shape Factor", &m_shape_factor, 0.1f, 2.0f);
}

geometry_shader_cube_sub_scene::geometry_shader_cube_sub_scene(
    geometry_shader_scene *_parent, const std::string &_shader_type,
    const std::string &_geometry_shader)
    : sub_scene<geometry_shader_scene>(_parent, _shader_type) {

  // Setup mesh
  prefab_cube::cube_mesh_data cube_mesh_data(
      prefab_cube::vertex_format::position_normal);
  m_cube_mesh_manager.setup_mesh(*cube_mesh_data.mesh);

  m_shader = new shader("shaders/geometry_shader_test/vertex_for_cube.shader",
                        "shaders/geometry_shader_test/fragment.shader",
                        _geometry_shader.c_str());
}

geometry_shader_cube_sub_scene::~geometry_shader_cube_sub_scene() {
  delete m_shader;
}

void geometry_shader_cube_sub_scene::render() {
  if (!m_shader) {
    return;
  }

  m_parent->set_matrices(m_shader);
  m_shader->use();
  m_shader->set_uniform("uShapeFactor", m_shape_factor);
  m_cube_mesh_manager.draw();
}

void geometry_shader_cube_sub_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Geometry Shader - %s", name().c_str());
  ImGui::Spacing();

  ImGui::SliderFloat("Shape Factor", &m_shape_factor, 0.1f, 2.0f);
}