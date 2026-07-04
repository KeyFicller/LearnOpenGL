#include "sketch_context.h"

#include "tests/cad/camera/sketch_camera.h"
#include "tests/cad/history/sketch_feature.h"
#include "tests/cad/instance.h"

namespace toy_cad {

sketch_context &sketch_context::instance() {
  static sketch_context s;
  return s;
}

sketch_feature *sketch_context::sketch() {
  if (!m_sketch_handle.valid()) {
    return nullptr;
  }
  return instance::get().db().try_get_as<sketch_feature>(m_sketch_handle);
}

const sketch_feature *sketch_context::sketch() const {
  if (!m_sketch_handle.valid()) {
    return nullptr;
  }
  return instance::get().db().try_get_as<sketch_feature>(m_sketch_handle);
}

sketch_context::camera_snapshot
sketch_context::capture_camera(const camera &cam) {
  camera_snapshot snap;
  snap.position = cam.Position;
  snap.world_up = cam.WorldUp;
  snap.front = cam.Front;
  snap.yaw = cam.Yaw;
  snap.pitch = cam.Pitch;
  snap.orthographic = cam.Orthographic;
  snap.left = cam.Left;
  snap.right = cam.Right;
  snap.bottom = cam.Bottom;
  snap.top = cam.Top;
  snap.aspect_ratio = cam.AspectRatio;
  snap.valid = true;
  return snap;
}

void sketch_context::apply_snapshot(camera &cam,
                                    const camera_snapshot &snap) {
  if (!snap.valid) {
    return;
  }
  cam.Position = snap.position;
  cam.WorldUp = snap.world_up;
  cam.Front = snap.front;
  cam.Yaw = snap.yaw;
  cam.Pitch = snap.pitch;
  cam.Orthographic = snap.orthographic;
  cam.Left = snap.left;
  cam.Right = snap.right;
  cam.Bottom = snap.bottom;
  cam.Top = snap.top;
  cam.set_aspect_ratio(snap.aspect_ratio);
  cam.update_view_matrix();
  cam.update_projection_matrix();
}

void sketch_context::begin(const gp_Ax2 &plane, handle sketch_handle,
                           camera &cam) {
  m_plane = plane;
  m_sketch_handle = sketch_handle;
  m_saved_camera = capture_camera(cam);
  m_active = true;

  align_camera_to_sketch_plane(cam, m_plane, cam.AspectRatio);

  if (auto *sketch_obj = sketch()) {
    sketch_obj->set_plane(m_plane);
  }
}

void sketch_context::finish() {
  if (!m_active) {
    return;
  }

  if (auto *sketch_obj = sketch()) {
    sketch_obj->regen();
    instance::get().history().push_back(m_sketch_handle);
  }

  m_active = false;
  m_sketch_handle = {};
  m_live_preview = {};
  m_plane_hover.reset();
}

void sketch_context::cancel() {
  if (!m_active) {
    return;
  }

  if (m_sketch_handle.valid()) {
    instance::get().db().destroy(m_sketch_handle);
    m_sketch_handle = {};
  }

  m_active = false;
  m_live_preview = {};
  m_plane_hover.reset();
  m_on_finish = {};
}

void sketch_context::apply_sketch_camera(camera &cam) const {
  if (!m_active) {
    return;
  }
  align_camera_to_sketch_plane(cam, m_plane, cam.AspectRatio);
}

void sketch_context::restore_camera(camera &cam) const {
  apply_snapshot(cam, m_saved_camera);
}

} // namespace toy_cad
