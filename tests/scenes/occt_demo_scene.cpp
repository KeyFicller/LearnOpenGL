#include "occt_demo_scene.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include "tests/component/shader_loader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef LEARNOPENGL_USE_OCCT
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <stdexcept>

namespace {
static glm::vec3 occt_point_to_gl(const gp_Pnt &p) {
  return glm::vec3(static_cast<float>(p.X()), static_cast<float>(p.Z()),
                   static_cast<float>(-p.Y()));
}

static void push_vertex(std::vector<float> &out, const glm::vec3 &pos,
                        const glm::vec3 &n) {
  out.push_back(pos.x);
  out.push_back(pos.y);
  out.push_back(pos.z);
  out.push_back(n.x);
  out.push_back(n.y);
  out.push_back(n.z);
  out.push_back(0.0f);
  out.push_back(0.0f);
}

static void append_flat_triangle(std::vector<float> &out, const glm::vec3 &a,
                                 const glm::vec3 &b, const glm::vec3 &c) {
  glm::vec3 n = glm::cross(b - a, c - a);
  const float len = glm::length(n);
  if (len > 1e-20f) {
    n /= len;
  } else {
    n = glm::vec3(0.0f, 1.0f, 0.0f);
  }
  push_vertex(out, a, n);
  push_vertex(out, b, n);
  push_vertex(out, c, n);
}
} // namespace
#endif

occt_demo_scene::occt_demo_scene()
    : renderable_scene_base("OCCT Demo") {
  m_light.Type = light_type::k_directional;
  m_light.Direction = glm::vec3(0.35f, 1.0f, 0.4f);
  m_light.SimpleLighting = true;
  m_light.SimpleColor = glm::vec3(1.0f);
  m_light.SimpleIntensity = 0.55f;
  const glm::vec3 light_base =
      m_light.SimpleColor * m_light.SimpleIntensity;
  m_light.Ambient = light_base * 0.22f;
  m_light.Diffuse = light_base;
  m_light.Specular = light_base * 0.45f;

  const glm::vec3 obj{0.25f, 0.72f, 0.92f};
  m_material.SimpleMaterial = true;
  m_material.SimpleAlbedo = obj;
  m_material.SimpleSpecular = 0.25f;
  m_material.Ambient = obj * 0.25f;
  m_material.Diffuse = obj;
  m_material.Specular = glm::vec3(0.25f);
  m_material.Shininess = 64.0f;
}

void occt_demo_scene::init(GLFWwindow *_window) {
  renderable_scene_base::init(_window);

#ifndef LEARNOPENGL_USE_OCCT
  m_status_message =
      "Open CASCADE is disabled. Configure CMake with "
      "LEARNOPENGL_OCCT_BUILD_DIR pointing to your OCCT build tree (folder that "
      "contains OpenCASCADETargets.cmake), then reconfigure.";
#else
  try {
    load_shader_pair("shaders/occt_demo/vertex.shader",
                     "shaders/occt_demo/fragment.shader",
                     "shaders/light_material_test/light_fragment.shader",
                     m_shader, m_light_shader);
  } catch (const std::exception &e) {
    m_status_message = e.what();
  }

  m_camera.Position = {0.0f, 0.0f, 4.0f};
  m_camera.Yaw = -90.0f;
  m_camera.Pitch = 0.0f;
  m_camera.update_view_matrix();

  if (m_shader) {
    try {
      build_mesh_from_occt();
      m_status_message = "Mesh: OCCT BRep box + sphere (fused) + BRepMesh.";
    } catch (const std::exception &e) {
      m_status_message = e.what();
    }
  }
#endif
}

void occt_demo_scene::build_mesh_from_occt() {
#ifdef LEARNOPENGL_USE_OCCT
  m_vertex_data.clear();
  m_triangle_count = 0;

  const double bx = static_cast<double>(m_box_size.x);
  const double by = static_cast<double>(m_box_size.y);
  const double bz = static_cast<double>(m_box_size.z);
  const double hbx = bx * 0.5;
  const double hby = by * 0.5;
  const double hbz = bz * 0.5;
  const gp_Pnt box_min(static_cast<double>(m_box_center.x) - hbx,
                       static_cast<double>(m_box_center.y) - hby,
                       static_cast<double>(m_box_center.z) - hbz);
  const gp_Pnt box_max(static_cast<double>(m_box_center.x) + hbx,
                       static_cast<double>(m_box_center.y) + hby,
                       static_cast<double>(m_box_center.z) + hbz);
  const TopoDS_Shape box_shape = BRepPrimAPI_MakeBox(box_min, box_max).Shape();

  TopoDS_Shape sphere_shape =
      BRepPrimAPI_MakeSphere(static_cast<double>(m_sphere_radius)).Shape();
  gp_Trsf sphere_trsf;
  sphere_trsf.SetTranslation(
      gp_Vec(static_cast<double>(m_sphere_center.x),
             static_cast<double>(m_sphere_center.y),
             static_cast<double>(m_sphere_center.z)));
  const TopoDS_Shape sphere_placed =
      BRepBuilderAPI_Transform(sphere_shape, sphere_trsf, true).Shape();

  BRepAlgoAPI_Fuse fuse(box_shape, sphere_placed);
  fuse.Build();
  if (!fuse.IsDone()) {
    throw std::runtime_error("OCCT: BRepAlgoAPI_Fuse failed.");
  }
  const TopoDS_Shape shape = fuse.Shape();

  BRepMesh_IncrementalMesh mesher(shape,
                                  static_cast<double>(m_mesh_deflection));

  for (TopExp_Explorer explorer(shape, TopAbs_FACE); explorer.More();
       explorer.Next()) {
    const TopoDS_Face &face = TopoDS::Face(explorer.Current());
    TopLoc_Location loc;
    const Handle(Poly_Triangulation) &tri =
        BRep_Tool::Triangulation(face, loc);
    if (tri.IsNull()) {
      continue;
    }
    const gp_Trsf trsf = loc.Transformation();
    const int n_triangles = tri->NbTriangles();

    for (int t = 1; t <= n_triangles; ++t) {
      const Poly_Triangle triangle = tri->Triangle(t);
      int i1 = 0;
      int i2 = 0;
      int i3 = 0;
      triangle.Get(i1, i2, i3);
      if (face.Orientation() == TopAbs_REVERSED) {
        std::swap(i2, i3);
      }

      const gp_Pnt p1 = tri->Node(i1).Transformed(trsf);
      const gp_Pnt p2 = tri->Node(i2).Transformed(trsf);
      const gp_Pnt p3 = tri->Node(i3).Transformed(trsf);

      const glm::vec3 a = occt_point_to_gl(p1);
      const glm::vec3 b = occt_point_to_gl(p2);
      const glm::vec3 c = occt_point_to_gl(p3);
      append_flat_triangle(m_vertex_data, a, b, c);
      ++m_triangle_count;
    }
  }

  if (m_vertex_data.empty()) {
    throw std::runtime_error("OCCT: triangulation produced no triangles.");
  }

  const size_t vertex_count = m_vertex_data.size() / 8;
  mesh_data data(m_vertex_data.data(),
                 m_vertex_data.size() * sizeof(float), vertex_count,
                 {{3, GL_FLOAT, GL_FALSE},
                  {3, GL_FLOAT, GL_FALSE},
                  {2, GL_FLOAT, GL_FALSE}});
  m_mesh.setup_mesh(data);
#endif
}

void occt_demo_scene::render() {
  if (!m_shader || m_vertex_data.empty()) {
    return;
  }

  m_shader->use();
  set_matrices(m_shader);
  uniform(*m_shader, m_light, "uLight");
  uniform(*m_shader, m_material, "uMaterial");
  m_shader->set_uniform("uEyePosition", m_camera.Position);
  m_mesh.draw();
}

void occt_demo_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Open CASCADE (OCCT)");
  ImGui::Spacing();

  if (!m_status_message.empty()) {
    ImGui::TextWrapped("%s", m_status_message.c_str());
  }

#ifdef LEARNOPENGL_USE_OCCT
  if (ImGui::DragFloat3("Box size", &m_box_size.x, 0.02f, 0.1f, 4.0f)) {
    try {
      build_mesh_from_occt();
    } catch (const std::exception &e) {
      m_status_message = e.what();
    }
  }
  if (ImGui::DragFloat3("Box center", &m_box_center.x, 0.02f)) {
    try {
      build_mesh_from_occt();
    } catch (const std::exception &e) {
      m_status_message = e.what();
    }
  }
  if (ImGui::DragFloat3("Sphere center", &m_sphere_center.x, 0.02f)) {
    try {
      build_mesh_from_occt();
    } catch (const std::exception &e) {
      m_status_message = e.what();
    }
  }
  if (ImGui::SliderFloat("Sphere radius", &m_sphere_radius, 0.15f, 2.0f)) {
    try {
      build_mesh_from_occt();
    } catch (const std::exception &e) {
      m_status_message = e.what();
    }
  }
  if (ImGui::SliderFloat("Mesh linear deflection", &m_mesh_deflection, 0.01f,
                         0.4f)) {
    try {
      build_mesh_from_occt();
    } catch (const std::exception &e) {
      m_status_message = e.what();
    }
  }
  ImGui::Text("Triangles: %zu", m_triangle_count);

  ImGui::Separator();
  ImGui::Text("Lighting & material");

  ImGui::PushID("occt_light");
  ui(m_light);
  ImGui::PopID();

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::PushID("occt_material");
  ui(m_material);
  ImGui::PopID();
#endif

  render_camera_ui();
}
