#include "viewport_datum.h"

#include "basic/shader.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/prefab_quad.h"

#include <vector>

viewport_datum &viewport_datum::instance() {
  static viewport_datum s;
  return s;
}

viewport_datum::viewport_datum() {
  std::vector<float> verts =
      prefab_quad::get_vertices(prefab_quad::vertex_format::position_only);
  std::vector<unsigned int> idx = prefab_quad::get_indices();
  const std::vector<vertex_attribute> attrs =
      prefab_quad::get_attributes(prefab_quad::vertex_format::position_only);

  m_quad = new mesh_manager();
  m_quad->setup_mesh(mesh_data(
      verts.data(), verts.size() * sizeof(float), idx.data(), idx.size(),
      attrs));
}

viewport_datum::~viewport_datum() { delete m_quad; }

void viewport_datum::draw(shader &shader_prog, const glm::mat4 &mvp,
                          const glm::vec4 &color) {
  shader_prog.set_uniform("uMVP", mvp);
  shader_prog.set_uniform("uColor", color);
  m_quad->draw();
}
