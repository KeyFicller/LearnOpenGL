#pragma once

#include "primitive_feature.h"
#include "tests/cad/database/handle.h"
#include <glm/glm.hpp>

#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

namespace toy_cad {

/**
 * Extrusion solid from a planar face profile along a direction.
 */
class extrude_feature : public primitive_feature {
public:
  extrude_feature() = default;

  void set_profile(const TopoDS_Face &face, const glm::vec3 &direction,
                   const glm::vec3 &anchor);
  void set_depth(float depth);
  void set_preview_mode(bool preview);

  [[nodiscard]] bool preview_mode() const { return m_preview_mode; }
  [[nodiscard]] glm::vec3 direction() const { return m_direction; }
  [[nodiscard]] glm::vec3 anchor() const { return m_anchor; }
  [[nodiscard]] bool has_geometry() const { return m_shape_ok; }

  void regen() override;

  void draw_global() override;
  void draw_local() override;
  void draw_ui(handle explorer_row) override;
  void inspect() const override;

  [[nodiscard]] handle brep_handle() const override { return m_brep; }

private:
  TopoDS_Face m_profile_face;
  bool m_has_profile = false;

  glm::vec3 m_direction{0.0f, 0.0f, 1.0f};
  glm::vec3 m_anchor{0.0f, 0.0f, 0.0f};
  float m_depth = 0.0f;

  TopoDS_Shape m_shape;
  bool m_shape_ok = false;
  handle m_brep{};

  struct render_mesh {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<unsigned int> indices;
    unsigned int vao = 0, vbo = 0, nbo = 0, ibo = 0;
    bool dirty = true;
  };
  mutable render_mesh m_mesh;

  void clear_brep_topology();
  void rebuild_brep_topology();

  void triangulate_shape() const;
  void upload_mesh() const;
  void draw_mesh() const;
};

} // namespace toy_cad
