#pragma once

#include <glm/glm.hpp>

#include <TopoDS_Face.hxx>
#include <TopAbs_Orientation.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

namespace toy_cad::geometry {

/** Unit normal from triangle edges (CCW front face: cross(p2-p1, p3-p1)). */
inline glm::vec3 normal_from_triangle(const glm::vec3 &p1, const glm::vec3 &p2,
                                      const glm::vec3 &p3) {
  const glm::vec3 n = glm::cross(p2 - p1, p3 - p1);
  const float len = glm::length(n);
  if (len < 1e-10f) {
    return glm::vec3(0.0f, 0.0f, 1.0f);
  }
  return n / len;
}

/** Swap triangle winding when the BRep face is reversed (OCCT → OpenGL CCW front). */
inline void apply_occt_triangle_winding(const TopoDS_Face &face, int &i1, int &i2,
                                        int &i3) {
  (void)i1;
  if (face.Orientation() == TopAbs_REVERSED) {
    std::swap(i2, i3);
  }
}

/**
 * Ensure CCW winding (cross(p2-p1, p3-p1)) matches the outward BRep normal.
 * Use after reading Poly_Triangle indices; works regardless of OCCT→GL axis mapping.
 */
inline void align_triangle_winding_to_outward(const glm::vec3 &p1,
                                              const glm::vec3 &p2,
                                              const glm::vec3 &p3, int &i2,
                                              int &i3,
                                              const glm::vec3 &outward) {
  if (glm::dot(normal_from_triangle(p1, p2, p3), outward) < 0.0f) {
    std::swap(i2, i3);
  }
}

/** Outward-pointing unit normal for a planar or curved BRep face. */
glm::vec3 face_outward_normal(const TopoDS_Face &face);

} // namespace toy_cad::geometry
