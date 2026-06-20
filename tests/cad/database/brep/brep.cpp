#include "brep.h"

#include "brep_edge.h"
#include "brep_face.h"
#include "brep_query.h"
#include "brep_shell.h"
#include "tests/cad/database/database.h"
#include "tests/cad/interaction/selection_manager.h"

#include "imgui.h"

#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

#include <cstdio>
#include <unordered_map>

namespace toy_cad {

void brep::clear(database &db) {
  for (const handle shell_h : m_shell_handles) {
    if (auto *shell = db.try_get_as<brep_shell>(shell_h)) {
      for (const handle face_h : shell->face_handles()) {
        db.destroy(face_h);
      }
      for (const handle edge_h : shell->edge_handles()) {
        db.destroy(edge_h);
      }
      db.destroy(shell_h);
    }
  }
  m_shell_handles.clear();
  m_bvh.clear();
}

void brep::rebuild(database &db, handle self_handle, const TopoDS_Shape &shape) {
  clear(db);
  interaction::selection_manager::instance().clear_selection();

  if (shape.IsNull() || !self_handle.valid()) {
    return;
  }

  const handle shell_h = db.emplace<brep_shell>(self_handle);
  auto *shell = db.try_get_as<brep_shell>(shell_h);
  if (!shell) {
    return;
  }

  for (TopExp_Explorer face_exp(shape, TopAbs_FACE); face_exp.More();
       face_exp.Next()) {
    const TopoDS_Face face = TopoDS::Face(face_exp.Current());
    const handle face_h = db.emplace<brep_face>(face, self_handle, shell_h);
    shell->add_face(face_h);
  }

  for (TopExp_Explorer edge_exp(shape, TopAbs_EDGE); edge_exp.More();
       edge_exp.Next()) {
    const TopoDS_Edge edge = TopoDS::Edge(edge_exp.Current());
    const handle edge_h = db.emplace<brep_edge>(edge, self_handle, shell_h);
    shell->add_edge(edge_h);
  }

  m_shell_handles.push_back(shell_h);
  rebuild_bvh(db);

  std::printf("[brep::rebuild] shell=%u faces=%zu edges=%zu bvh=%s\n",
              shell_h.index, shell->face_handles().size(),
              shell->edge_handles().size(), m_bvh.empty() ? "empty" : "built");
}

void brep::rebuild_bvh(database &db) {
  std::vector<handle> primitives;
  for_each_subshape_handle(db, [&](handle h) { primitives.push_back(h); });

  std::unordered_map<uint64_t, geometry::aabb> bounds_cache;
  m_bvh.build(primitives, [&db, &bounds_cache](handle h) -> const geometry::aabb * {
    const uint64_t key =
        (static_cast<uint64_t>(h.index) << 32u) | h.generation;
    if (const auto it = bounds_cache.find(key); it != bounds_cache.end()) {
      return &it->second;
    }
    if (const auto bounds = subshape_bounds(db, h)) {
      const auto inserted = bounds_cache.emplace(key, *bounds);
      return &inserted.first->second;
    }
    return nullptr;
  });
}

void brep::for_each_subshape_handle(database &db,
                                    const std::function<void(handle)> &fn) const {
  if (!fn) {
    return;
  }

  for (const handle shell_h : m_shell_handles) {
    const auto *shell = db.try_get_as<brep_shell>(shell_h);
    if (!shell) {
      continue;
    }
    for (const handle face_h : shell->face_handles()) {
      fn(face_h);
    }
    for (const handle edge_h : shell->edge_handles()) {
      fn(edge_h);
    }
  }
}

std::optional<geometry::ray_hit>
brep::intersect_ray(database &db, const glm::vec3 &ray_origin,
                    const glm::vec3 &ray_dir) const {
  if (!has_bvh()) {
    return std::nullopt;
  }

  return m_bvh.intersect_ray(
      ray_origin, ray_dir,
      [&db](handle h, const glm::vec3 &origin, const glm::vec3 &dir) -> float {
        if (auto *face = try_get_face(db, h)) {
          return face->intersect_ray(origin, dir);
        }
        if (auto *edge = try_get_edge(db, h)) {
          return edge->intersect_ray(origin, dir);
        }
        return -1.0f;
      });
}

void brep::intersect_frustum(database &db, const glm::vec4 frustum_planes[6],
                             std::vector<handle> &out_hits) const {
  if (!has_bvh()) {
    out_hits.clear();
    return;
  }
  m_bvh.intersect_frustum(frustum_planes, out_hits);
  (void)db;
}

void brep::draw_subshape_highlighted(database &db, handle subshape,
                                     const glm::vec3 &color) const {
  toy_cad::draw_subshape_highlighted(db, subshape, color);
}

size_t brep::face_count(database &db) const {
  size_t count = 0;
  for (const handle shell_h : m_shell_handles) {
    const auto *shell = db.try_get_as<brep_shell>(shell_h);
    if (shell) {
      count += shell->face_handles().size();
    }
  }
  return count;
}

size_t brep::edge_count(database &db) const {
  size_t count = 0;
  for (const handle shell_h : m_shell_handles) {
    const auto *shell = db.try_get_as<brep_shell>(shell_h);
    if (shell) {
      count += shell->edge_handles().size();
    }
  }
  return count;
}

void brep::inspect() const {
  object::inspect();
  ImGui::SeparatorText("brep");
  ImGui::BulletText("Shells: %zu", m_shell_handles.size());
  ImGui::BulletText("BVH: %s", m_bvh.empty() ? "empty" : "built");
}

} // namespace toy_cad
