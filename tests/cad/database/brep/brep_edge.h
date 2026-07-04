#pragma once

#include "tests/cad/database/handle.h"
#include "tests/cad/database/object.h"
#include "tests/cad/geometry/subshape_handle.h"

#include <TopoDS_Edge.hxx>
#include <glm/glm.hpp>

namespace toy_cad {

class brep_edge final : public object {
public:
  brep_edge() = default;
  brep_edge(const TopoDS_Edge &edge, handle owner_brep, handle owner_shell);

  [[nodiscard]] const TopoDS_Edge &edge() const { return m_edge; }
  [[nodiscard]] const geometry::edge_geometry &geom() const { return m_geom; }
  [[nodiscard]] const geometry::aabb &bounds() const { return m_geom.bounds; }
  [[nodiscard]] handle owner_brep() const { return m_owner_brep; }
  [[nodiscard]] handle owner_shell() const { return m_owner_shell; }

  void set_topology(const TopoDS_Edge &edge, handle owner_brep,
                    handle owner_shell);

  [[nodiscard]] float intersect_ray(const glm::vec3 &ray_origin,
                                    const glm::vec3 &ray_dir) const;

  void draw_highlighted(const glm::vec3 &color) const;

  void inspect() const override;

private:
  TopoDS_Edge m_edge;
  geometry::edge_geometry m_geom;
  handle m_owner_brep;
  handle m_owner_shell;

  void build_geometry();
};

} // namespace toy_cad
