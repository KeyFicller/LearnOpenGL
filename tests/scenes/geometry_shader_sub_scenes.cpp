#include "geometry_shader_sub_scenes.h"
#include "geometry_shader_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include <glm/gtc/matrix_transform.hpp>

// ============================================================================
// geometry_shader_line_strip_sub_scene
// ============================================================================

geometry_shader_sub_scene::geometry_shader_sub_scene(
    geometry_shader_scene *_parent, const std::string &_shader_type,
    const std::string &_geometry_shader)
    : sub_scene<geometry_shader_scene>(_parent, _shader_type) {
  m_shader = new shader("shaders/geometry_shader_test/vertex.shader",
                        "shaders/geometry_shader_test/fragment.shader",
                        _geometry_shader.c_str());
}

geometry_shader_sub_scene::~geometry_shader_sub_scene() { delete m_shader; }

void geometry_shader_sub_scene::render() {
  if (!m_shader) {
    return;
  }

  m_parent->set_matrices(m_shader);
  m_shader->use();
  m_parent->m_points_mesh_manager.bind();
  m_shader->set_uniform("uShapeFactor", m_shape_factor);
  glDrawArrays(GL_POINTS, 0, m_parent->m_points_mesh_manager.get_index_count());
}

void geometry_shader_sub_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Geometry Shader - Line Strip");
  ImGui::Spacing();

  ImGui::SliderFloat("Shape Factor", &m_shape_factor, 0.1f, 2.0f);
}