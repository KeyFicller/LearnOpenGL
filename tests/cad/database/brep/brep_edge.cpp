#include "brep_edge.h"

#include "tests/cad/instance.h"

#include "basic/shader.h"
#include "glad/gl.h"
#include "imgui.h"

#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <gp_Pnt.hxx>

namespace toy_cad {

brep_edge::brep_edge(const TopoDS_Edge &edge, handle owner_brep,
                     handle owner_shell)
    : m_edge(edge), m_owner_brep(owner_brep), m_owner_shell(owner_shell) {
  build_geometry();
}

void brep_edge::set_topology(const TopoDS_Edge &edge, handle owner_brep,
                             handle owner_shell) {
  m_edge = edge;
  m_owner_brep = owner_brep;
  m_owner_shell = owner_shell;
  build_geometry();
}

void brep_edge::build_geometry() {
  m_geom.vertices.clear();
  m_geom.bounds = geometry::aabb{};

  BRepAdaptor_Curve curve_adaptor(m_edge);
  const double first_param = curve_adaptor.FirstParameter();
  const double last_param = curve_adaptor.LastParameter();

  GCPnts_QuasiUniformDeflection discretizer;
  discretizer.Initialize(curve_adaptor, 0.1, first_param, last_param);

  if (discretizer.IsDone() && discretizer.NbPoints() > 0) {
    const int n_points = discretizer.NbPoints();
    m_geom.vertices.reserve(static_cast<size_t>(n_points));
    for (int i = 1; i <= n_points; ++i) {
      const gp_Pnt p = discretizer.Value(i);
      m_geom.vertices.emplace_back(static_cast<float>(p.X()),
                                   static_cast<float>(p.Y()),
                                   static_cast<float>(p.Z()));
    }
  } else {
    gp_Pnt p1 = curve_adaptor.Value(first_param);
    gp_Pnt p2 = curve_adaptor.Value(last_param);
    m_geom.vertices.emplace_back(static_cast<float>(p1.X()),
                                 static_cast<float>(p1.Y()),
                                 static_cast<float>(p1.Z()));
    m_geom.vertices.emplace_back(static_cast<float>(p2.X()),
                                 static_cast<float>(p2.Y()),
                                 static_cast<float>(p2.Z()));
  }

  if (!m_geom.vertices.empty()) {
    m_geom.bounds.min = m_geom.vertices[0];
    m_geom.bounds.max = m_geom.vertices[0];
    for (const auto &v : m_geom.vertices) {
      m_geom.bounds.expand(v);
    }
    const glm::vec3 padding(0.001f);
    m_geom.bounds.min -= padding;
    m_geom.bounds.max += padding;
  }
}

float brep_edge::intersect_ray(const glm::vec3 & /*ray_origin*/,
                               const glm::vec3 & /*ray_dir*/) const {
  return -1.0f;
}

void brep_edge::draw_highlighted(const glm::vec3 &color) const {
  if (m_geom.vertices.size() < 2) {
    return;
  }

  GLuint vao = 0, vbo = 0;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, m_geom.vertices.size() * sizeof(glm::vec3),
               m_geom.vertices.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

  shader &sh = instance::get().viewport_shader();
  sh.use();
  sh.set_uniform("uMVP", instance::get().disp().clip_from_world());
  sh.set_uniform("uColor", glm::vec4(color, 1.0f));

  glBindVertexArray(vao);
  glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(m_geom.vertices.size()));
  glBindVertexArray(0);

  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
}

void brep_edge::inspect() const {
  object::inspect();
  ImGui::SeparatorText("brep_edge");
  ImGui::BulletText("Polyline points: %zu", m_geom.vertices.size());
}

} // namespace toy_cad
