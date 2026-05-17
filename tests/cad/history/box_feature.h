#pragma once

#include "primitive_feature.h"
#include <glm/glm.hpp>

#include <BRepBuilderAPI_MakeSolid.hxx>
#include <TopoDS_Shape.hxx>
#include <memory>

namespace toy_cad {

/**
 * Axis-aligned box primitive.
 * Stores two diagonal corners, generates OCCT topology on regen().
 */
class box_feature : public primitive_feature {
public:
  box_feature() = default;

  /** Set corner points (axis-aligned, opposite corners). */
  void set_corners(const glm::vec3 &p1, const glm::vec3 &p2);
  [[nodiscard]] glm::vec3 corner_min() const { return m_min; }
  [[nodiscard]] glm::vec3 corner_max() const { return m_max; }
  [[nodiscard]] bool has_geometry() const { return m_shape_ok; }

  /** Rebuild OCCT topology from current corners. */
  void regen() override;

  void draw_global() override;
  void draw_local() override;
  void draw_ui(handle explorer_row) override;
  void inspect() const override;

private:
  glm::vec3 m_min{0.0f, 0.0f, 0.0f};
  glm::vec3 m_max{0.0f, 0.0f, 0.0f};

  // OCCT topology
  TopoDS_Shape m_shape;
  bool m_shape_ok = false;

  // OpenGL render data (mesh triangulation)
  struct render_mesh {
    std::vector<float> vertices; // position (3 floats per vertex)
    std::vector<float> normals;  // normal (3 floats per vertex)
    std::vector<unsigned int> indices;
    unsigned int vao = 0, vbo = 0, nbo = 0, ibo = 0;
    bool dirty = true;
  };
  mutable render_mesh m_mesh;

  void triangulate_shape() const;
  void upload_mesh() const;
  void draw_mesh() const;
};

} // namespace toy_cad
