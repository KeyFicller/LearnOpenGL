#include "occt_mesh_utils.h"

#include <BRepAdaptor_Surface.hxx>

namespace toy_cad::geometry {

glm::vec3 face_outward_normal(const TopoDS_Face &face) {
  BRepAdaptor_Surface surf(face);
  const double u =
      0.5 * (surf.FirstUParameter() + surf.LastUParameter());
  const double v =
      0.5 * (surf.FirstVParameter() + surf.LastVParameter());

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

} // namespace toy_cad::geometry
