#include "advanced_glsl_sub_scenes.h"
#include "advanced_glsl_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include "tests/component/mesh_manager.h"
#include <glm/gtc/matrix_transform.hpp>

// ============================================================================
// stencil_object_outline_sub_scene
// ============================================================================

advanced_glsl_vertex_variable_sub_scene::
    advanced_glsl_vertex_variable_sub_scene(advanced_glsl_scene *parent)
    : sub_scene(parent, "Vertex Variable") {

  std::vector<float> vertices = {
      -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, -0.5f, 0.0f,
      0.0f,  0.0f,  1.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f,  1.0f,
  };
  mesh_data point_mesh_data{vertices.data(),
                            sizeof(vertices[0]) * vertices.size(),
                            vertices.size() / 6,
                            {
                                {3, GL_FLOAT, false},
                                {3, GL_FLOAT, false},
                            }};
  m_point_mesh.setup_mesh(point_mesh_data);
}

void advanced_glsl_vertex_variable_sub_scene::render() {
  auto *p = parent();
  if (!p->m_shader) {
    return;
  }

  // Draw normal object and write to stencil buffer
  glEnable(GL_PROGRAM_POINT_SIZE);
  p->m_shader->use();
  p->set_matrices(p->m_shader);
  p->m_shader->set_uniform("uSubSenceIndex", 0);
  p->m_shader->set_uniform("uPointSize", m_point_size);
  p->m_shader->set_uniform("uIncrementPointSize", (int)m_increment_point_size);
  m_point_mesh.bind();
  glDrawArrays(GL_POINTS, 0, m_point_mesh.get_index_count());
  glDisable(GL_PROGRAM_POINT_SIZE);
}

void advanced_glsl_vertex_variable_sub_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Vertex Variable Parameters");

  ImGui::SliderFloat("Point Size", &m_point_size, 0.1f, 10.0f);
  ImGui::Checkbox("Increment Point Size", &m_increment_point_size);
}

advanced_glsl_fragment_variable_sub_scene::
    advanced_glsl_fragment_variable_sub_scene(advanced_glsl_scene *parent)
    : sub_scene(parent, "Fragment Variable") {
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  m_split_base = (float)viewport[2] * 0.5f;
}

void advanced_glsl_fragment_variable_sub_scene::render() {
  auto *p = parent();
  if (!p->m_shader) {
    return;
  }

  p->m_shader->use();
  p->set_matrices(p->m_shader);
  p->m_shader->set_uniform("uSubSenceIndex", 1);
  p->m_shader->set_uniform("uSplitScreen", (int)m_split_screen);
  p->m_shader->set_uniform("uSplitBase", m_split_base);
  p->m_shader->set_uniform("uFrontFacing", (int)m_front_facing);
  p->m_mesh.draw();
}

void advanced_glsl_fragment_variable_sub_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Fragment Variable Parameters");
  if (ImGui::Checkbox("Split Screen", &m_split_screen)) {
    m_front_facing = !m_split_screen;
  }
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  ImGui::SliderFloat("Split Base", &m_split_base, 0.0f, (float)viewport[2]);
  if (ImGui::Checkbox("Front Facing", &m_front_facing)) {
    m_split_screen = !m_front_facing;
  }
}

advanced_glsl_uniform_buffer_sub_scene::advanced_glsl_uniform_buffer_sub_scene(
    advanced_glsl_scene *parent)
    : sub_scene(parent, "Uniform Buffer") {

  glGenBuffers(1, &m_ubo_handle);
  glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_handle);
  glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr,
               GL_STATIC_DRAW);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  glBindBufferRange(GL_UNIFORM_BUFFER, 0, m_ubo_handle, 0,
                    2 * sizeof(glm::mat4));
}

void advanced_glsl_uniform_buffer_sub_scene::render() {
  auto *p = parent();
  if (!p->m_shader) {
    return;
  }

  glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_handle);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4),
                  &p->m_camera.m_view_matrix);
  glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4),
                  &p->m_camera.m_projection_matrix);
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  for (const auto &object : m_objects) {
    object->Shader->use();
    object->Shader->set_uniform("uSubSenceIndex", 2);
    object->Shader->set_uniform("uObjectColor", object->ObjectColor);
    object->Shader->set_uniform(
        "model", glm::translate(glm::mat4(1.0f), object->Position));
    p->m_mesh.draw();
  }
}

void advanced_glsl_uniform_buffer_sub_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Uniform Buffer Parameters");
  if (ImGui::Button("Add Object")) {
    m_objects.push_back(std::make_unique<object_info>());
    m_objects.back()->Shader = new shader(
        "shaders/advanced_glsl_test/vertex_with_uniform_buffer.shader",
        "shaders/advanced_glsl_test/fragment.shader");
    unsigned int uniform_blk_index = glGetUniformBlockIndex(
        m_objects.back()->Shader->id(), "uCameraMatrics");
    if (uniform_blk_index != GL_INVALID_INDEX) {
      glUniformBlockBinding(m_objects.back()->Shader->id(), uniform_blk_index,
                            0);
    }
    m_objects.back()->Position =
        glm::vec3(rand() % 10 - 5, rand() % 10 - 5, rand() % 10 - 5);
    m_objects.back()->ObjectColor = glm::vec3(
        rand() % 100 / 100.0f, rand() % 100 / 100.0f, rand() % 100 / 100.0f);
  }

  if (ImGui::Button("Delete Object")) {
    m_objects.pop_back();
  }
}