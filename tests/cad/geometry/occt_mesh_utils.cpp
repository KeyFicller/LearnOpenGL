#include "occt_mesh_utils.h"

#include <BRepAdaptor_Surface.hxx>
#include <Poly_Triangulation.hxx>
#include <gp_Pnt2d.hxx>

namespace toy_cad::geometry {

glm::vec3 face_outward_normal_at_uv(const TopoDS_Face &face, double u,
                                    double v) {
  BRepAdaptor_Surface surf(face);

  gp_Pnt p;
  gp_Vec du, dv;
  surf.D1(u, v, p, du, dv);
  gp_Vec n = du.Crossed(dv);

  if (n.Magnitude() < 1e-10) {
    return glm::vec3(0.0f, 0.0f, 1.0f);
  }

  n.Normalize();
  if (face.Orientation() == TopAbs_REVERSED) {
    n.Reverse();
  }

  return glm::vec3(static_cast<float>(n.X()), static_cast<float>(n.Y()),
                   static_cast<float>(n.Z()));
}

glm::vec3 face_outward_normal(const TopoDS_Face &face) {
  BRepAdaptor_Surface surf(face);
  const double u =
      0.5 * (surf.FirstUParameter() + surf.LastUParameter());
  const double v =
      0.5 * (surf.FirstVParameter() + surf.LastVParameter());
  return face_outward_normal_at_uv(face, u, v);
}

glm::vec3 mesh_node_outward_normal(const TopoDS_Face &face,
                                   const Handle(Poly_Triangulation) &tri,
                                   int node_index) {
  if (!tri.IsNull() && tri->HasUVNodes()) {
    const gp_Pnt2d uv = tri->UVNode(node_index);
    return face_outward_normal_at_uv(face, uv.X(), uv.Y());
  }
  return face_outward_normal(face);
}

glm::vec3 mesh_triangle_winding_reference(const TopoDS_Face &face,
                                          const Handle(Poly_Triangulation) &tri,
                                          int n1, int n2, int n3) {
  if (!tri.IsNull() && tri->HasUVNodes()) {
    const gp_Pnt2d uv1 = tri->UVNode(n1);
    const gp_Pnt2d uv2 = tri->UVNode(n2);
    const gp_Pnt2d uv3 = tri->UVNode(n3);
    const double u = (uv1.X() + uv2.X() + uv3.X()) / 3.0;
    const double v = (uv1.Y() + uv2.Y() + uv3.Y()) / 3.0;
    return face_outward_normal_at_uv(face, u, v);
  }
  return face_outward_normal(face);
}

} // namespace toy_cad::geometry
