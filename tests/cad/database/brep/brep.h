#pragma once

#include "tests/cad/database/handle.h"
#include "tests/cad/database/object.h"
#include "tests/cad/geometry/bvh_tree.h"

#include <TopoDS_Shape.hxx>
#include <glm/glm.hpp>

#include <functional>
#include <optional>
#include <vector>

namespace toy_cad {

class database;

class brep final : public object {
public:
  brep() = default;

  [[nodiscard]] const std::vector<handle> &shell_handles() const {
    return m_shell_handles;
  }
  [[nodiscard]] const geometry::bvh_tree &bvh() const { return m_bvh; }
  [[nodiscard]] bool has_bvh() const { return !m_bvh.empty(); }
  [[nodiscard]] bool empty() const { return m_shell_handles.empty(); }

  /** Destroy child shell/face/edge objects and clear BVH. */
  void clear(database &db);

  /** Rebuild topology from an OCCT shape (destroys previous children). */
  void rebuild(database &db, handle self_handle, const TopoDS_Shape &shape);

  [[nodiscard]] std::optional<geometry::ray_hit>
  intersect_ray(database &db, const glm::vec3 &ray_origin,
                const glm::vec3 &ray_dir) const;

  void intersect_frustum(database &db, const glm::vec4 frustum_planes[6],
                         std::vector<handle> &out_hits) const;

  void for_each_subshape_handle(
      database &db,
      const std::function<void(handle)> &fn) const;

  void draw_subshape_highlighted(database &db, handle subshape,
                                 const glm::vec3 &color) const;

  [[nodiscard]] size_t face_count(database &db) const;
  [[nodiscard]] size_t edge_count(database &db) const;

  void inspect() const override;

private:
  std::vector<handle> m_shell_handles;
  geometry::bvh_tree m_bvh;

  void rebuild_bvh(database &db);
};

} // namespace toy_cad
